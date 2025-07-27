export module jizhak_.thread_pool_.utilities_tpm;

import std;
import jizhak.error;
import jizhak.thread_pool.task;
import jizhak.thread_pool.worker;

export namespace jzh {
    struct GlobalTaskRegistry {
    protected:
        std::unordered_map<Task::id_t, TaskPointer> tasks_{};

        mutable std::mutex mutex_{};

    public:
        GlobalTaskRegistry() = default;
        GlobalTaskRegistry(const GlobalTaskRegistry &) = delete;
        GlobalTaskRegistry(GlobalTaskRegistry &&) noexcept = default;

        GlobalTaskRegistry &operator=(const GlobalTaskRegistry &) = delete;
        GlobalTaskRegistry &operator=(GlobalTaskRegistry &&) noexcept = default;

        virtual ~GlobalTaskRegistry() = default;

        explicit GlobalTaskRegistry(const TaskPointer& task_ptr) : tasks_{ {task_ptr->task_info.id, task_ptr} } {}

        virtual TaskPointer operator()(const TaskPointer& task) {
            return add_task(task);
        }

        virtual std::optional<JizhakError> operator()(const Task::id_t id) {
            return this->remove_task(id);
        }

        [[nodiscard]] virtual TaskPointer operator[](const Task::id_t id) {
            std::scoped_lock lock(mutex_);
            return tasks_[id];
        }

        [[nodiscard]] virtual TaskPointer add_task(const TaskPointer& task) {
            std::scoped_lock lock(mutex_);
            return tasks_[task->task_info.id] = task;
        }

        virtual void add_task(const std::map<Task::id_t, TaskPointer>& tasks) {
            std::scoped_lock lock(mutex_);
            tasks_.insert(tasks.begin(), tasks.end());
        }

        virtual std::optional<JizhakError> remove_task(const Task::id_t id) {
            std::scoped_lock lock(mutex_);
            if (tasks_.erase(id) > 0)
                return std::nullopt;

            return JizhakError(JizhakErrorID::task_not_found);
        }

        virtual void remove_task(const std::vector<Task::id_t>& ids) {
            std::scoped_lock lock(mutex_);
            for (const auto& id : ids)
                if (const auto it = tasks_.find(id); it != tasks_.end())
                    tasks_.erase(it);;
        }

        [[nodiscard]] virtual std::unordered_map<unsigned long long, TaskPointer>::iterator find_task(const Task::id_t id) {
            std::scoped_lock lock(mutex_);
            return tasks_.find(id);
        }

        [[nodiscard]] virtual std::unordered_map<unsigned long long, TaskPointer>::const_iterator find_task(
            const Task::id_t id) const {
            std::scoped_lock lock(mutex_);
            return tasks_.find(id);
        }

        [[nodiscard]] virtual TaskPointer get_task(const Task::id_t id) {
            std::scoped_lock lock(mutex_);
            return tasks_.at(id);
        }

        [[nodiscard]] virtual std::optional<TaskPointer> get_task(const Task::id_t id) const {
            std::scoped_lock lock(mutex_);
            if (const auto it = tasks_.find(id); it != tasks_.end())
                return it->second;

            return std::nullopt;
        }

        [[nodiscard]] virtual size_t size() const {
            std::scoped_lock lock(mutex_);
            return tasks_.size();
        }

        [[nodiscard]] virtual bool empty() const {
            std::scoped_lock lock(mutex_);
            return tasks_.empty();
        }

        virtual void clear() {
            std::scoped_lock lock(mutex_);
            tasks_.clear();
        }
    };

    struct InformationWorkerTable : GlobalTaskRegistry {
    protected:
        std::shared_ptr<BaseWorker> worker_ptr_{};

        size_t total_tasks_ = 0;
        size_t async_tasks_ = 0;

    public:
        InformationWorkerTable() = default;

        explicit InformationWorkerTable(const std::shared_ptr<BaseWorker>& worker_ptr) : worker_ptr_(worker_ptr) {}

        BaseWorker& operator->() const {
            std::scoped_lock lock(mutex_);

            return *worker_ptr_;
        }

        TaskPointer add_task(const TaskPointer& task) override {
            std::scoped_lock lock(mutex_);

            ++total_tasks_;
            if (task->task_info.is_async) ++async_tasks_;

            return tasks_[task->task_info.id] = task;
        }

        void add_task(const std::map<Task::id_t, TaskPointer>& tasks) override {
            std::scoped_lock lock(mutex_);

            total_tasks_ += tasks.size();
            for (const auto& id : tasks)
                if (id.second.task_ptr->task_info.is_async) ++async_tasks_;

            tasks_.insert(tasks.begin(), tasks.end());
        }

        std::optional<JizhakError> remove_task(const Task::id_t id) override {
            std::scoped_lock lock(mutex_);

            const auto it = tasks_.find(id);
            if (it == tasks_.end())
                return JizhakError(JizhakErrorID::task_not_found);

            --total_tasks_;
            if (it->second->task_info.is_async) --async_tasks_;

            tasks_.erase(it);
            return std::nullopt;
        }

        void remove_task(const std::vector<Task::id_t>& ids) override {
            std::scoped_lock lock(mutex_);

            for (const auto& id : ids)
                if (const auto it = tasks_.find(id); it != tasks_.end()) {
                    --total_tasks_;
                    if (it->second->task_info.is_async) --async_tasks_;

                    tasks_.erase(it);
                }
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

        [[nodiscard]] size_t total_tasks() const {
            std::scoped_lock lock(mutex_);
            return total_tasks_;
        }

        [[nodiscard]] size_t async_tasks() const {
            std::scoped_lock lock(mutex_);
            return async_tasks_;
        }
    };

    struct InformationWorkerTableSorter {
        size_t pending_tasks{};
        std::jthread::id id{};

        auto operator<=>(const InformationWorkerTableSorter& other) const = default;
    };
}