export module jizhak.thread_pool.info;

import std;
import jizhak.error;
import jizhak.thread_pool.task;
import jizhak.thread_pool.worker;

export namespace jzh {
    struct InformationTable {
    private:
        std::shared_ptr<BaseWorker> worker_ptr_{};

        size_t total_tasks_ = 0;
        size_t async_tasks_ = 0;

        std::map<Task::id_t, TaskInfo> tasks_{};

        mutable std::mutex mutex_{};

    public:
        InformationTable() = default;

        explicit InformationTable(const std::shared_ptr<BaseWorker>& worker_ptr) : worker_ptr_(worker_ptr) {}

        explicit InformationTable(const std::shared_ptr<BaseWorker>& worker_ptr, const TaskInfo& task)
            : worker_ptr_(worker_ptr) {
            add_task(task);
        }

        BaseWorker& operator->() const {
            std::scoped_lock lock(mutex_);

            return *worker_ptr_;
        }

        const TaskInfo& operator[](const Task::id_t id) const {
            return tasks_.at(id);
        }

        void operator()(const TaskInfo& task) {
            return this->add_task(task);
        }

        std::optional<JizhakError> operator()(const Task::id_t id) {
            return this->remove_task(id);
        }

        void add_task(const TaskInfo& task) {
            std::scoped_lock lock(mutex_);

            ++total_tasks_;
            if (task.is_async) ++async_tasks_;
            this->tasks_[task.id] = task;
        }

        std::optional<JizhakError> remove_task(const Task::id_t id) {
            std::scoped_lock lock(mutex_);

            if (const auto it = tasks_.find(id); it != tasks_.end()) {
                --total_tasks_;
                if (it->second.is_async) --async_tasks_;
                tasks_.erase(it);
                return std::nullopt;
            }
            return JizhakError(JizhakErrorID::task_not_found);
        }

        [[nodiscard]] std::optional<TaskInfo> get_task_info(Task::id_t id) const {
            std::scoped_lock lock(mutex_);

            const auto it = tasks_.find(id);

            if (it != tasks_.end()) {
                return it->second;
            }

            return std::nullopt;
        }

        [[nodiscard]] auto find_task(const Task::id_t id) {
            std::scoped_lock lock(mutex_);
            return tasks_.find(id);
        }

        [[nodiscard]] auto find_task(const Task::id_t id) const {
            std::scoped_lock lock(mutex_);
            return tasks_.find(id);
        }

        [[nodiscard]] size_t total_tasks() const {
            std::scoped_lock lock(mutex_);

            return total_tasks_;
        }

        [[nodiscard]] size_t async_tasks() const {
            std::scoped_lock lock(mutex_);

            return async_tasks_;
        }

        [[nodiscard]] std::shared_ptr<BaseWorker> get_worker() {
            std::scoped_lock lock(mutex_);

            return worker_ptr_;
        }

        void set_worker(const std::shared_ptr<BaseWorker>& worker_ptr) {
            std::scoped_lock lock(mutex_);

            worker_ptr_ = worker_ptr;
        }

        [[nodiscard]] const std::map<Task::id_t, TaskInfo>& tasks() const {
            return tasks_;
        }
    };

    struct InformationSorter {
        size_t pending_tasks{};
        std::jthread::id id{};

        auto operator<=>(const InformationSorter& other) const = default;
    };
}