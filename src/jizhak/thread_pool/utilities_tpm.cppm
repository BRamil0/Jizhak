// utilities_tpm.cppm

export module jizhak.thread_pool.utilities_tpm;

import jizhak.std;
import jizhak.error;
import jizhak.thread_pool.task;
import jizhak.thread_pool.worker;
import jizhak.thread_pool.tpm_base;

export namespace jzh {
    struct TaskRegistry {
    protected:
        std::unordered_map<Task::id_t, TaskPointer> tasks_{};

        mutable std::mutex mutex_{};

    public:
        TaskRegistry() = default;
        TaskRegistry(const TaskRegistry& other) = delete;
        TaskRegistry(TaskRegistry&& other) noexcept {
            std::scoped_lock lock(other.mutex_);
            tasks_ = std::move(other.tasks_);
        }

        TaskRegistry& operator=(const TaskRegistry& other) = delete;
        TaskRegistry& operator=(TaskRegistry&& other) noexcept {
            if (this != &other) {
                std::scoped_lock lock(mutex_, other.mutex_);
                tasks_ = std::move(other.tasks_);
                other.tasks_.clear();
            }
            return *this;
        }
        virtual ~TaskRegistry() = default;

        explicit TaskRegistry(const TaskPointer& task_ptr) : tasks_{ {task_ptr->task_info.id, task_ptr} } {}

        virtual std::expected<TaskPointer, JizhakError> operator()(const TaskPointer& task) {
            return add_task(task);
        }

        virtual std::optional<JizhakError> operator()(const Task::id_t id) {
            return this->remove_task(id);
        }

        [[nodiscard]] virtual TaskPointer operator[](const Task::id_t id) {
            std::scoped_lock lock(mutex_);
            return tasks_[id];
        }

        virtual std::expected<TaskPointer, JizhakError> add_task(const TaskPointer& task) {
            std::scoped_lock lock(mutex_);
            return tasks_[task->task_info.id] = task;
        }

        virtual std::optional<JizhakError> add_task(const std::map<Task::id_t, TaskPointer>& tasks) {
            std::scoped_lock lock(mutex_);
            tasks_.insert(tasks.begin(), tasks.end());
            return std::nullopt;
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

        [[nodiscard]] virtual std::optional<TaskPointer> find_task(Task::id_t id) const {
            std::scoped_lock lock(mutex_);
            auto it = tasks_.find(id);
            if (it != tasks_.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        [[nodiscard]] virtual std::expected<TaskPointer, JizhakError> find_task_for_key(const Task::id_t id) const {
            std::scoped_lock lock(mutex_);
            if (const auto it = tasks_.find(id); it != tasks_.end())
                return it->second;
            return std::unexpected<JizhakError>(JizhakErrorID::task_not_found);
        }

        [[nodiscard]] virtual std::optional<TaskPointer> get_task(const Task::id_t id) {
            std::scoped_lock lock(mutex_);
            if (const auto it = tasks_.find(id); it != tasks_.end())
                return it->second;

            return std::nullopt;
        }

        [[nodiscard]] virtual std::optional<TaskPointer> get_task(const Task::id_t id) const {
            std::scoped_lock lock(mutex_);
            if (const auto it = tasks_.find(id); it != tasks_.end())
                return it->second;

            return std::nullopt;
        }

        [[nodiscard]] virtual std::unordered_map<Task::id_t, TaskPointer> get_all_task() const {
            return std::unordered_map{tasks_};
        }

        [[nodiscard]] virtual TaskPointer set_task(const TaskPointer new_task) {
            std::scoped_lock lock(mutex_);
            return tasks_[new_task->task_info.id] = new_task;
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

        virtual std::unordered_map<Task::id_t, TaskPointer>::iterator begin_task() {
            std::scoped_lock lock(mutex_);
            return tasks_.begin();
        }

        virtual std::unordered_map<Task::id_t, TaskPointer>::const_iterator begin_task() const {
            std::scoped_lock lock(mutex_);
            return tasks_.end();
        }

        virtual std::unordered_map<Task::id_t, TaskPointer>::iterator end_task() {
            std::scoped_lock lock(mutex_);
            return tasks_.end();
        }

        virtual std::unordered_map<Task::id_t, TaskPointer>::const_iterator end_task() const {
            std::scoped_lock lock(mutex_);
            return tasks_.end();
        }
    };

    struct WorkerInformationTable : TaskRegistry {
    protected:
        std::shared_ptr<BaseWorker> worker_ptr_{};

        size_t total_tasks_ = 0;
        size_t async_tasks_ = 0;

        std::atomic<bool> is_shutdown_ = false;

    public:
        WorkerInformationTable() = default;

        explicit WorkerInformationTable(const std::shared_ptr<BaseWorker>& worker_ptr) : worker_ptr_(worker_ptr) {}

        WorkerInformationTable(const WorkerInformationTable&) = delete;
        WorkerInformationTable& operator=(const WorkerInformationTable&) = delete;

        WorkerInformationTable(WorkerInformationTable&& other) noexcept
            : TaskRegistry(std::move(other)),
              worker_ptr_(std::move(other.worker_ptr_)),
              total_tasks_(other.total_tasks_),
              async_tasks_(other.async_tasks_),
              is_shutdown_(other.is_shutdown_.load())
        {
            other.total_tasks_ = 0;
            other.async_tasks_ = 0;
        }

        WorkerInformationTable& operator=(WorkerInformationTable&& other) noexcept {
            if (this != &other) {
                TaskRegistry::operator=(std::move(other));
                worker_ptr_ = std::move(other.worker_ptr_);
                total_tasks_ = other.total_tasks_;
                async_tasks_ = other.async_tasks_;
                is_shutdown_.store(other.is_shutdown_.load());
                other.total_tasks_ = 0;
                other.async_tasks_ = 0;
            }
            return *this;
        }

        ~WorkerInformationTable() override = default;

        std::expected<TaskPointer, JizhakError> add_task(const TaskPointer& task) override {
            if (is_shutdown_.load(std::memory_order_relaxed))
                return std::unexpected<JizhakError>(JizhakErrorID::shutting);

            std::scoped_lock lock(mutex_);
            ++total_tasks_;
            if (task->task_info.is_async) ++async_tasks_;

            worker_ptr_->add_task(task);

            return tasks_[task->task_info.id] = task;
        }

        std::optional<JizhakError> add_task(const std::map<Task::id_t, TaskPointer>& tasks) override {
            if (is_shutdown_.load(std::memory_order_relaxed))
                return JizhakError(JizhakErrorID::shutting);

            std::scoped_lock lock(mutex_);
            total_tasks_ += tasks.size();
            for (const auto& values : tasks | std::views::values) {
                if (values->task_info.is_async)
                    ++async_tasks_;
                worker_ptr_->add_task(values);
            }

            tasks_.insert(tasks.begin(), tasks.end());
            return std::nullopt;
        }

        std::optional<JizhakError> remove_task(const Task::id_t id) override {
            std::scoped_lock lock(mutex_);

            const auto it = tasks_.find(id);
            if (it == tasks_.end())
                return JizhakError(JizhakErrorID::task_not_found);

            --total_tasks_;
            if (it->second->task_info.is_async) --async_tasks_;

            worker_ptr_->remove_task(id);
            tasks_.erase(it);
            return std::nullopt;
        }

        void remove_task(const std::vector<Task::id_t>& ids) override {
            std::scoped_lock lock(mutex_);

            for (const auto& id : ids)
                if (const auto it = tasks_.find(id); it != tasks_.end()) {
                    --total_tasks_;
                    if (it->second->task_info.is_async) --async_tasks_;
                    worker_ptr_->remove_task(id);
                    tasks_.erase(it);
                }
        }

        std::optional<JizhakError> task_completed(const Task::id_t id) {
            std::scoped_lock lock(mutex_);

            const auto it = tasks_.find(id);
            if (it == tasks_.end())
                return JizhakError(JizhakErrorID::task_not_found);

            --total_tasks_;
            if (it->second->task_info.is_async) --async_tasks_;

            tasks_.erase(it);
            return std::nullopt;
        }

        [[nodiscard]] TaskPointer set_task(const TaskPointer new_task) override {
            std::scoped_lock lock(mutex_);
            if (const auto it = tasks_.find(new_task->task_info.id); it == tasks_.end()) {
                ++total_tasks_;
                if (new_task->task_info.is_async) ++async_tasks_;
            }

            return tasks_[new_task->task_info.id] = new_task;
        }

        void mark_for_shutdown() {
            is_shutdown_.store(true, std::memory_order_release);
        }

        bool is_shutdown() {
            return is_shutdown_.load(std::memory_order_acquire);
        }

        void notify() const {
            worker_ptr_->notify();
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

        [[nodiscard]] WorkerInfo get_workers_info() const {
            std::scoped_lock lock(mutex_);

            return WorkerInfo{
                .id               = this->worker_ptr_->get_id(),
                .total_tasks      = this->total_tasks_,
                .async_tasks      = this->async_tasks_,
                .is_shutting_down = this->is_shutdown_.load(std::memory_order_relaxed), // FIX: read from atomic
                .tasks            = this->tasks_,
            };
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

    struct WorkerInformationTableSorter {
        size_t pending_tasks{};
        std::jthread::id id{};

        auto operator<=>(const WorkerInformationTableSorter& other) const = default;
    };

    template <typename TWorkerInfoTableSorter = WorkerInformationTableSorter, typename TWorkerInfoTable = WorkerInformationTable>
    struct WorkerRegistry {
    public:
        friend ThreadPoolManagerBase;

        using Workers = std::map<TWorkerInfoTableSorter, TWorkerInfoTable>;

        using WorkerSorter              = TWorkerInfoTableSorter;

        using TWorkerInfoTableSorter_T  = TWorkerInfoTableSorter;
        using TTWorkerInfoTable_T       = TWorkerInfoTable;

    private:
        Workers workers_{};
        mutable std::mutex mutex_{};

    public:
        WorkerRegistry() = default;
        WorkerRegistry(const WorkerRegistry &) = delete;
        WorkerRegistry(WorkerRegistry &&) noexcept = default;

        WorkerRegistry &operator=(const WorkerRegistry &) = delete;
        WorkerRegistry &operator=(WorkerRegistry &&) noexcept = default;

        virtual ~WorkerRegistry() = default;

        explicit WorkerRegistry(const Workers& worker_ptr) : workers_{ worker_ptr } {}

        std::jthread::id register_worker(WorkerSorter worker_sorter, TWorkerInfoTable worker_info) {
            std::scoped_lock lock(mutex_);
            const auto id = worker_info.get_worker()->get_id();
            this->workers_.emplace(std::move(worker_sorter), std::move(worker_info));
            return id;
        }

        std::expected<std::pair<WorkerSorter, WorkerInfo>, JizhakError> unregister_worker(std::jthread::id thread_id) {
            std::scoped_lock lock(mutex_);
            auto it = std::find_if(workers_.begin(), workers_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });

            if (it == workers_.end())
                return std::unexpected<JizhakError>(JizhakErrorID::worker_not_found);

            auto node = workers_.extract(it);

            return std::make_pair(std::move(node.key()), std::move(node.mapped()));
        }

        std::optional<std::shared_ptr<BaseWorker>> shutdown_worker(std::jthread::id thread_id) {
            std::scoped_lock lock(mutex_);
            auto it = std::find_if(workers_.begin(), workers_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });

            if (it == workers_.end()) {
                return std::nullopt;
            }

            it->second.mark_for_shutdown();
            return it->second.get_worker();
        }

        void final_unregister(std::jthread::id thread_id) {
            std::scoped_lock lock(mutex_);
            auto it = std::find_if(workers_.begin(), workers_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });
            if (it != workers_.end()) {
                workers_.erase(it);
            }
        }

        std::optional<JizhakError> on_task_completed(Task::id_t task_id, std::jthread::id thread_id) {
            std::scoped_lock lock(mutex_);

            auto it = std::find_if(workers_.begin(), workers_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });

            if (it == workers_.end()) {
                return std::nullopt;
            }

            if(auto result = it->second.task_completed(task_id); result.has_value()) {
                return result;
            }

            auto node = workers_.extract(it);
            if (node.key().pending_tasks > 0)
                --node.key().pending_tasks;

            workers_.insert(std::move(node));

            return std::nullopt;
        }

        std::optional<JizhakError> transfer_task_between_workers(Task::id_t task_id,
                                                                 std::jthread::id from_worker_id,
                                                                 std::jthread::id to_worker_id) {
            std::scoped_lock lock(mutex_);

            auto from_it = std::find_if(workers_.begin(), workers_.end(),
                [&](const auto& pair){ return pair.first.id == from_worker_id; });

            auto to_it = std::find_if(workers_.begin(), workers_.end(),
                [&](const auto& pair){ return pair.first.id == to_worker_id; });

            if (from_it == workers_.end() || to_it == workers_.end()) {
                return JizhakError(JizhakErrorID::worker_not_found);
            }

            auto task_ptr_opt = from_it->second.get_task(task_id);
            if (task_ptr_opt.has_value()) {
                return JizhakError(JizhakErrorID::task_not_found);
            }

            auto from_node = workers_.extract(from_it);

            to_it = std::find_if(workers_.begin(), workers_.end(),
                [&](const auto& pair){ return pair.first.id == to_worker_id; });
            if (to_it == workers_.end()) {
                workers_.insert(std::move(from_node));
                return JizhakError(JizhakErrorID::worker_not_found);
            }
            auto to_node = workers_.extract(to_it);

            from_node.mapped().remove_task(task_id);
            [[maybe_unused]] auto _ = to_node.mapped().add_task(*task_ptr_opt);

            if (from_node.key().pending_tasks > 0) --from_node.key().pending_tasks;
            ++to_node.key().pending_tasks;

            workers_.insert(std::move(from_node));
            workers_.insert(std::move(to_node));

            return std::nullopt;
        }

        std::optional<std::deque<TaskPointer>> yield_tasks_from(std::jthread::id victim_id) {
            std::scoped_lock lock(mutex_);
            auto it = std::find_if(workers_.begin(), workers_.end(),
                                   [&](const auto& pair) { return pair.first.id == victim_id; });

            if (it != workers_.end() && !it->second.is_shutdown())
                return it->second.get_worker()->yield_half_of_tasks();

            return std::nullopt;
        }

        std::vector<std::shared_ptr<BaseWorker>> get_all_worker_pointers() {
            std::scoped_lock lock(mutex_);

            std::vector<std::shared_ptr<BaseWorker>> pointers;
            pointers.reserve(workers_.size());

            for (const auto& pair : workers_)
                pointers.push_back(pair.second.get_worker());
            return pointers;
        }

        [[nodiscard]] std::expected<WorkerInfo, JizhakError> get_worker_info(std::jthread::id thread_id) const {
            std::scoped_lock lock(mutex_);

            auto it = std::find_if(workers_.begin(), workers_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });

            if (it == workers_.end())
                return std::unexpected(JizhakError(JizhakErrorID::worker_not_found));

            return it->second.get_workers_info();
        }

        [[nodiscard]] std::map<std::jthread::id, WorkerInfo> get_workers_info() const {
            std::scoped_lock lock(mutex_);
            std::map<std::jthread::id, WorkerInfo> workers_info{};
            for (const auto& worker : workers_)
                workers_info.emplace(worker.first.id, worker.second.get_workers_info());
            return workers_info;
        }

        [[nodiscard]] const Workers& get_workers_ptr() const {
            std::scoped_lock lock(mutex_);
            return workers_;
        }

        [[nodiscard]] Workers set_workers_ptr(const Workers& worker_ptr) {
            std::scoped_lock lock(mutex_);
            std::swap(this->workers_, worker_ptr);
            return worker_ptr;
        }

        std::expected<Task::id_t, JizhakError> add_task(TaskPointer task) {
            std::scoped_lock lock(mutex_);
            if (workers_.empty()) {
                return std::unexpected(JizhakError(JizhakErrorID::no_workers_available));
            }

            task->task_info.status = TaskStatus::waiting;

            auto it_to_assign = workers_.begin();
            if (task->task_info.performer_worker_id) {
                auto specific_it = std::find_if(workers_.begin(), workers_.end(),
                    [&](const auto& pair) {
                        return pair.first.id == task->task_info.performer_worker_id.value();
                    });

                if (specific_it != workers_.end())
                    it_to_assign = specific_it;
                else
                    task->task_info.performer_worker_id = std::nullopt;
            }

            while (it_to_assign != workers_.end() && it_to_assign->second.is_shutdown()) {
                ++it_to_assign;
            }

            if (it_to_assign == workers_.end()) {
                return std::unexpected(JizhakError(JizhakErrorID::no_workers_available));
            }

            auto node_handle = workers_.extract(it_to_assign);

            TWorkerInfoTable& info_table_obj = node_handle.mapped();

            ++node_handle.key().pending_tasks;
            workers_.insert(std::move(node_handle));

            (void)info_table_obj.add_task(task);
            task->task_info.current_thread_id = info_table_obj.get_worker()->get_id();

            return task->task_info.id;
        }

        std::optional<JizhakError> remove_task(Task::id_t task_id) {
            std::scoped_lock lock(mutex_);

            for (auto it = workers_.begin(); it != workers_.end(); ++it) {
                if (!it->second.remove_task(task_id)) {
                    auto node_handle = workers_.extract(it);
                    if (node_handle.key().pending_tasks > 0)
                        --node_handle.key().pending_tasks;

                    workers_.insert(std::move(node_handle));
                    return std::nullopt;
                }
            }

            return JizhakError(JizhakErrorID::task_not_found); // Не знайшли ні в кого
        }

        [[nodiscard]] std::optional<TaskPointer> find_task_in_any_worker(Task::id_t task_id) const {
            std::scoped_lock lock(mutex_);
            for (const auto& pair : workers_) {
                if (auto task_opt = pair.second.find_task(task_id)) {
                    return task_opt;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::expected<std::vector<std::jthread::id>, JizhakError> get_all_worker_ids() const {
            std::scoped_lock lock(mutex_);
            if (workers_.empty())
                return std::unexpected<JizhakError>(JizhakErrorID::empty);

            std::vector<std::jthread::id> ids;
            ids.reserve(workers_.size());
            for (const auto& pair : workers_) {
                ids.push_back(pair.first.id);
            }
            return ids;
        }

        void notify_all() {
            std::scoped_lock lock(mutex_);
            for (const auto& worker : workers_) {
                worker.second.notify();
            }
        }

        std::optional<JizhakError> notify(std::jthread::id thread_id) {
            std::scoped_lock lock(mutex_);
            auto it = std::find_if(workers_.begin(), workers_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });

            if (it == workers_.end())
                return JizhakError(JizhakErrorID::worker_not_found);

            it->second.notify();

            return std::nullopt;
        }

        [[nodiscard]] virtual size_t size() const {
            std::scoped_lock lock(mutex_);
            return workers_.size();
        }

        [[nodiscard]] virtual bool empty() const {
            std::scoped_lock lock(mutex_);
            return workers_.empty();
        }

        virtual void clear() {
            std::scoped_lock lock(mutex_);
            workers_.clear();
        }
    };
}