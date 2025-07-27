export module jizhak_.thread_pool_.utilities_tpm;

import std;
import jizhak.error;
import jizhak.thread_pool.task;
import jizhak.thread_pool.worker;

export namespace jzh {
    struct InformationWorkerTable {
    private:
        std::shared_ptr<BaseWorker> worker_ptr_{};

        size_t total_tasks_ = 0;
        size_t async_tasks_ = 0;

        std::map<Task::id_t, TaskPointer> tasks_{};

        mutable std::mutex mutex_{};

    public:
        InformationWorkerTable() = default;

        explicit InformationWorkerTable(const std::shared_ptr<BaseWorker>& worker_ptr) : worker_ptr_(worker_ptr) {}

        explicit InformationWorkerTable(const std::shared_ptr<BaseWorker>& worker_ptr, const TaskPointer& task)
            : worker_ptr_(worker_ptr) {
            add_task(task);
        }

        BaseWorker& operator->() const {
            std::scoped_lock lock(mutex_);

            return *worker_ptr_;
        }

        const TaskPointer& operator[](const Task::id_t id) const {
            return tasks_.at(id);
        }

        void operator()(const TaskPointer& task) {
            return this->add_task(task);
        }

        std::optional<JizhakError> operator()(const Task::id_t id) {
            return this->remove_task(id);
        }

        void add_task(const TaskPointer& task) {
            std::scoped_lock lock(mutex_);

            ++total_tasks_;
            if (task->task_info.is_async) ++async_tasks_;
            this->tasks_[task->task_info.id] = task;
        }

        std::optional<JizhakError> remove_task(const Task::id_t id) {
            std::scoped_lock lock(mutex_);

            if (const auto it = tasks_.find(id); it != tasks_.end()) {
                --total_tasks_;
                if (it->second->task_info.is_async) --async_tasks_;
                tasks_.erase(it);
                return std::nullopt;
            }
            return JizhakError(JizhakErrorID::task_not_found);
        }

        [[nodiscard]] std::optional<TaskPointer> get_task(Task::id_t id) const {
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

        [[nodiscard]] std::shared_ptr<BaseWorker> get_worker() const {
            std::scoped_lock lock(mutex_);

            return worker_ptr_;
        }

        void set_worker(const std::shared_ptr<BaseWorker>& worker_ptr) {
            std::scoped_lock lock(mutex_);

            worker_ptr_ = worker_ptr;
        }

        [[nodiscard]] const std::map<Task::id_t, TaskPointer>& tasks() const {
            return tasks_;
        }
    };

    struct InformationSorter {
        size_t pending_tasks{};
        std::jthread::id id{};

        auto operator<=>(const InformationSorter& other) const = default;
    };
}