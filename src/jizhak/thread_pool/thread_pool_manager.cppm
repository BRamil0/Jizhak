export module jizhak.thread_pool.tpm;
import std;

import jizhak.thread_pool.tpm_base;
export import jizhak.thread_pool.task;
export import jizhak.thread_pool.worker;
export import jizhak.thread_pool.info;
export import jizhak.thread_pool.this_tpm;
export import jizhak.error;

export namespace jzh::concepts {
    template <typename TInfoTable, typename TWorker> concept is_supported_info_table = requires(
        TInfoTable table, const jzh::TaskInfo& task_info, const std::shared_ptr<TWorker>& worker_ptr) {
        { TInfoTable(worker_ptr, task_info) };
        { table.add_task(task_info) } -> std::same_as<void>;
        { table.remove_task(task_info.id) } -> std::same_as<std::optional<jzh::JizhakError>>;
        { table.get_worker() } -> std::same_as<std::shared_ptr<jzh::BaseWorker>>;
        { table.operator->() } -> std::same_as<jzh::BaseWorker&>;
        };

    template <typename TWorker> concept is_supported_worker = requires(TWorker worker, jzh::Task task) {
        requires std::is_base_of_v<jzh::BaseWorker, TWorker>; requires std::default_initializable<TWorker>;
        { worker.start(std::function<void(std::stop_token)>()) } -> std::same_as<void>;
        { worker.add_task(std::move(task)) } -> std::same_as<void>;
        { worker.notify() } -> std::same_as<void>; { worker.start_shutdown() } -> std::same_as<void>;
        { worker.join() } -> std::same_as<void>; { worker.instant_stop() } -> std::same_as<void>;
        { worker.get_id() } -> std::same_as<std::jthread::id>;
    };
} // namespace jzh::concepts

export namespace jzh {
    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    class ThreadPoolManager : public ThreadPoolManagerBase,
                              public std::enable_shared_from_this<ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>> {
    public:
        friend class BaseWorker;

        using InfoTable     = std::map<TInfoSorter, TInfoTable>;
        using OptionalError = std::optional<JizhakError>;

    private:
        InfoTable info_table_{};

        std::atomic<size_t> pending_tasks_{0};

        std::atomic<bool> pause_ = false;

        mutable std::mutex info_table_mutex_{};

        mutable std::mutex wait_mutex_;
        std::condition_variable wait_cv_;

    protected:
        std::jthread::id create_worker() {
            auto worker_ptr = std::make_shared<TWorker>();

            auto work_function = [this, worker_raw = worker_ptr.get()](std::stop_token token) {
                this_thread::set_tpm(std::static_pointer_cast<ThreadPoolManagerBase>(this->shared_from_this()));
                worker_raw->run_loop(token);
            };
            worker_ptr->start(std::move(work_function));

            this->info_table_.emplace(TInfoSorter{ .id = worker_ptr->get_id() }, worker_ptr);

            return worker_ptr->get_id();
        }

        std::expected<std::vector<std::jthread::id>, JizhakError> create_worker(unsigned int quantity) {
            if (quantity == 0) return std::unexpected<JizhakError>(JizhakErrorID::zero_transferred);

            std::vector<std::jthread::id> ids{};
            ids.reserve(quantity);

            for ([[maybe_unused]] auto _ : std::ranges::iota_view{0u, quantity})
                ids.push_back(this->create_worker());
            return ids;
        }


        OptionalError stop_worker(std::jthread::id thread_id) {
            std::shared_ptr<BaseWorker> worker_to_stop;

            {
                std::scoped_lock lock(info_table_mutex_);
                auto it = std::find_if(info_table_.begin(), info_table_.end(), [&](const auto& pair) {
                    return pair.first.id == thread_id;
                });

                if (it == info_table_.end()) return JizhakError(JizhakErrorID::worker_not_found);

                worker_to_stop = it->second.get_worker();
                info_table_.erase(it);
            }

            worker_to_stop->start_shutdown();
            worker_to_stop->join();

            return std::nullopt;
        }


        std::expected<std::weak_ptr<BaseWorker>, JizhakError> get_worker(std::jthread::id thread_id) override {
            if (auto it = info_table_.find(TInfoSorter{ .id = thread_id }); it != info_table_.end()) {
                return it->second.get_worker();
            }
            return std::unexpected<JizhakError>(JizhakErrorID::worker_not_found);
        }


        std::expected<std::weak_ptr<BaseWorker>, JizhakError> get_worker_by_index(size_t index) override {
            std::scoped_lock lock(info_table_mutex_);
            if (index >= info_table_.size()) return std::unexpected<JizhakError>(JizhakErrorID::index_overrun);

            auto it = info_table_.begin();
            std::advance(it, index);
            return it->second.get_worker();
        }


        std::expected<Task::id_t, JizhakError> create_task(Task& task, TaskInfo& task_info) {
            if (info_table_.empty())
                throw std::runtime_error("no workers available");

            if (task.id != task_info.id)
                return std::unexpected(JizhakError(JizhakErrorID::identifiers_are_different));

            auto first_entry_iterator = info_table_.begin();

            auto node_handle           = info_table_.extract(first_entry_iterator);
            TInfoTable& info_table_obj = node_handle.mapped();

            ++node_handle.key().pending_tasks;

            info_table_.insert(std::move(node_handle));

            info_table_obj.add_task(task_info);

            ++pending_tasks_;

            info_table_obj.get_worker()->add_task(std::move(task));

            return task.id;
        }


        OptionalError delete_task(Task::id_t task_id) {
            for (auto& [sorter, info_table] : info_table_)
                if (info_table.find_task(task_id) != info_table.tasks().end())
                    return this->delete_task(task_id, sorter.id);

            return JizhakError(JizhakErrorID::task_not_found);
        }

        OptionalError delete_task(Task::id_t task_id, std::jthread::id thread_id) {
            auto it = std::find_if(info_table_.begin(), info_table_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });

            if (it != info_table_.end()) return it->second.remove_task(task_id);

            return JizhakError(JizhakErrorID::task_not_found);
        }


        OptionalError task_completed(Task::id_t task_id, std::jthread::id thread_id) override {
            std::scoped_lock lock(info_table_mutex_);

            auto it = std::find_if(info_table_.begin(), info_table_.end(), [&](const auto& pair) {
                return pair.first.id == thread_id;
            });

            if (it == info_table_.end()) return JizhakError{JizhakErrorID::worker_not_found};

            auto& info_table_obj = it->second;
            info_table_obj.remove_task(task_id);

            auto node = info_table_.extract(it);
            --node.key().pending_tasks;
            info_table_.insert(std::move(node));

            if (pending_tasks_.fetch_sub(1) == 1) wait_cv_.notify_all();

            return std::nullopt;
        }


        OptionalError notify_steal_task(Task::id_t task_id, std::jthread::id to_thread_id,
                                        std::jthread::id from_thread_id) override {
            std::scoped_lock lock(info_table_mutex_);

            auto from_it = std::find_if(info_table_.begin(), info_table_.end(), [&](const auto& pair) {
                return pair.first.id == from_thread_id;
            });

            auto to_it = std::find_if(info_table_.begin(), info_table_.end(), [&](const auto& pair) {
                return pair.first.id == to_thread_id;
            });

            if (from_it == info_table_.end() || to_it == info_table_.end()) {
                return JizhakError{JizhakErrorID::worker_not_found};
            }

            TaskInfo stolen_task_info;
            if (auto task_opt = from_it->second.get_task_info(task_id)) {
                stolen_task_info = *task_opt;
                from_it->second.remove_task(task_id);
            } else {
                return JizhakError{JizhakErrorID::task_not_found};
            }

            to_it->second.add_task(stolen_task_info);

            auto from_node = info_table_.extract(from_it);
            --from_node.key().pending_tasks;
            info_table_.insert(std::move(from_node));

            auto to_it_new = std::find_if(info_table_.begin(), info_table_.end(), [&](const auto& pair) {
                return pair.first.id == to_thread_id;
            });
            if (to_it_new == info_table_.end()) return JizhakError{JizhakErrorID::internal_error};

            auto to_node = info_table_.extract(to_it_new);
            ++to_node.key().pending_tasks;
            info_table_.insert(std::move(to_node));

            return std::nullopt;
        }


        size_t __number_workers() const override {
            return this->number_workers();
        }


        bool __is_paused() const override {
            return this->is_paused();
        }

        std::expected<Task::id_t, JizhakError> __add_task(Task& task, TaskInfo& task_info) override {
            std::scoped_lock lock(info_table_mutex_);
            return this->create_task(task, task_info);
        }

    public:
        explicit ThreadPoolManager() = default;

        explicit ThreadPoolManager(unsigned int quantity) {
            if (auto result = this->add_worker(quantity); !result)
                throw std::runtime_error("Failed to create initial workers: " + std::string(result.error().what()));
        }

        ThreadPoolManager(const ThreadPoolManager&) = delete;
        ThreadPoolManager(ThreadPoolManager&&)      = default;

        ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
        ThreadPoolManager& operator=(ThreadPoolManager&&)      = default;

        ~ThreadPoolManager() override {
            try {
                this->stop_all(std::chrono::minutes(10));
            }
            catch (...) {
                this->instant_stop_all();
            }
        }


        std::expected<const InfoTable&, JizhakError> operator[]() const {
            return this->get_info_table();
        }


        std::expected<const TInfoTable&, JizhakError> operator[](std::jthread::id thread_id) const {
            return this->search_worker_for(thread_id);
        }


        std::expected<const TaskInfo, JizhakError> operator[](const Task::id_t task_id) const {
            return this->search_task_for(task_id);
        }


        std::expected<Task::id_t, JizhakError> operator()(Task& task, TaskInfo& task_info) {
            return this->create_task(task, task_info);
        }


        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(std::tuple<Task, TaskInfo> task_bundle) {
            return this->add_task(std::forward<std::tuple<Task, TaskInfo>>(task_bundle));
        }


        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(TaskInfo task_info, F&& func, Args&&... args) {
            return this->add_task(std::forward<TaskInfo>(task_info), std::forward<F>(func),
                                  std::forward<Args>(args)...);
        }

        template <typename F, typename... Args>
        requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(F&& func, Args&&... args) {
            return this->add_task(std::forward<F>(func), std::forward<Args>(args)...);
        }

        std::expected<std::jthread::id, JizhakError> add_worker() {
            std::scoped_lock lock(info_table_mutex_);
            return this->create_worker();
        }

        std::expected<std::vector<std::jthread::id>, JizhakError> add_worker(unsigned int quantity) {
            std::scoped_lock lock(info_table_mutex_);
            return this->create_worker(quantity);
        }


        OptionalError remove_worker(std::jthread::id thread_id) {
            return this->stop_worker(thread_id);
        }


        std::expected<Task::id_t, JizhakError> add_task(Task& task, TaskInfo& task_info) {
            std::scoped_lock lock(info_table_mutex_);
            return this->create_task(task, task_info);
        }


        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(std::tuple<Task, TaskInfo> task_bundle) {
            std::scoped_lock lock(info_table_mutex_);

            return std::apply([this](Task& task, TaskInfo& task_info) {
                return this->create_task(task, task_info);
            }, task_bundle);
        }

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(const TaskInfo task_info, F&& func, Args&&... args) {
            using ReturnType = std::invoke_result_t<F, Args...>;

            auto bound_func = std::bind_front(std::forward<F>(func), std::forward<Args>(args)...);
            std::packaged_task<ReturnType()> packaged_task(std::move(bound_func));

            std::future<ReturnType> future_result = packaged_task.get_future();
            Task::Function work_function          = [pt = std::move(packaged_task)]() mutable {
                pt();
            };

            auto [task, new_task_info] = make_task(std::move(work_function), task_info);

            std::scoped_lock lock(info_table_mutex_);

            if (auto creation_result = create_task(task, new_task_info);
                !creation_result) return std::unexpected(creation_result.error());

            return std::make_tuple(std::move(future_result), new_task_info.id);
        }

        template <typename F, typename... Args>
        requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(F&& func, Args&&... args) {
            return this->add_task(task_info_set_id(TaskInfo()), std::forward<F>(func), std::forward<Args>(args)...);
        }

        OptionalError remove_task(Task::id_t task_id) {
            std::scoped_lock lock(info_table_mutex_);
            return this->delete_task(task_id);
        }


        OptionalError remove_task(Task::id_t task_id, std::jthread::id thread_id) {
            std::scoped_lock lock(info_table_mutex_);
            return this->delete_task(task_id, thread_id);
        }


        void wait_all() {
            std::unique_lock lock(wait_mutex_);

            wait_cv_.wait(lock, [this] {
                return pending_tasks_.load() == 0;
            });
        }


        template <class Rep, class Period>
        OptionalError wait_all(const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock lock(wait_mutex_);

            bool success = wait_cv_.wait_for(lock, timeout, [this] {
                return pending_tasks_.load() == 0;
            });

            if (success) return std::nullopt;
            return JizhakError{JizhakErrorID::timeout_expired};
        }


        OptionalError stop_all() {
            wait_all();

            std::vector<TInfoSorter> keys_to_stop;
            {
                std::scoped_lock lock(info_table_mutex_);
                for (const auto& pair : info_table_) {
                    keys_to_stop.push_back(pair.first);
                }
            }

            for (const auto& key : keys_to_stop) {
                stop_worker(key.id);
            }

            return std::nullopt;
        }


        template <class Rep, class Period> OptionalError stop_all(
            const std::chrono::duration<Rep, Period>& timeout) {
            auto future = std::async(std::launch::async,
                static_cast<OptionalError (ThreadPoolManager::*)()>(&ThreadPoolManager::stop_all),
                this
            );

            if (future.wait_for(timeout) == std::future_status::ready) return future.get();

            this->instant_stop_all();

            throw std::runtime_error("Timeout expired during thread pool shutdown.");
        }


        void instant_stop_all() {
            std::vector<std::shared_ptr<BaseWorker>> workers_to_join;
            {
                std::scoped_lock lock(info_table_mutex_);
                for (const auto& pair : info_table_)
                    workers_to_join.push_back(pair.second.get_worker());

            }

            for (const auto& worker_ptr : workers_to_join)
                worker_ptr->instant_stop();


            notify_all();

            for (const auto& worker_ptr : workers_to_join) {
                worker_ptr->join();
            }

            {
                std::scoped_lock lock(info_table_mutex_);
                info_table_.clear();
            }
        }


        void pause() {
            this->pause_ = true;
        }


        void resume() {
            this->pause_ = false;
            this->notify_all();
        }


        void notify_all() {
            std::scoped_lock lock(info_table_mutex_);
            for (const auto& info_ptr : info_table_) {
                info_ptr.second.get_worker()->notify();
            }
        }


        void notify(std::jthread::id thread_id) {
            std::scoped_lock lock(info_table_mutex_);
            if (auto it = info_table_.find(TInfoSorter{ .id = thread_id }); it != info_table_.end())
                it->second->notify();
        }


        std::expected<const InfoTable&, JizhakError> get_info_table() const {
            return this->info_table_;
        }


        std::expected<const TInfoTable&, JizhakError> search_worker_for(std::jthread::id thread_id) const {
            std::scoped_lock lock(info_table_mutex_);

            for (const auto& pair : info_table_) if (pair.first.id == thread_id) return pair.second;

            return std::unexpected(JizhakError{JizhakErrorID::worker_not_found});
        }


        std::expected<const TaskInfo, JizhakError> search_task_for(Task::id_t task_id) const {
            std::scoped_lock lock(info_table_mutex_);

            for (auto&& info : info_table_ | std::views::values)
                if (auto it = info.find_task(task_id); it != info.tasks().end()) return it->second;

            return std::unexpected(JizhakError{JizhakErrorID::task_not_found});
        }


        [[nodiscard]] size_t size() const {
            return this->number_workers();
        }


        [[nodiscard]] size_t number_tasks() const {
            return this->pending_tasks_;
        }


        [[nodiscard]] size_t number_workers() const {
            std::scoped_lock lock(info_table_mutex_);
            return this->info_table_.size();
        }


        [[nodiscard]] bool is_there_task() const {
            if (this->pending_tasks_) return true;
            return false;
        }


        [[nodiscard]] bool is_paused() const {
            return this->pause_;
        }
    };

    using DefaultThreadPoolManager = ThreadPoolManager<>;

    inline std::shared_ptr<DefaultThreadPoolManager> new_tpm() {
        return std::make_shared<DefaultThreadPoolManager>();
    }

    inline std::shared_ptr<DefaultThreadPoolManager> new_tpm(unsigned int quantity) {
        return std::make_shared<DefaultThreadPoolManager>(quantity);
    }
} // namespace jzh
