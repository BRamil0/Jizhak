export module jizhak.thread_pool.tpm;
import std;

import jizhak.thread_pool.tpm_base;
export import jizhak.thread_pool.task;
export import jizhak.thread_pool.worker;
export import jizhak.thread_pool.info;
export import jizhak.error;

export namespace jzh::concepts {
    template <typename TInfoTable, typename TWorker>
    concept is_supported_info_table = requires(TInfoTable table, const jzh::TaskInfo& task_info, const std::shared_ptr<TWorker>& worker_ptr) {
        { TInfoTable(worker_ptr, task_info) };

        { table.add_task(task_info) } -> std::same_as<void>;
        { table.remove_task(task_info.id) } -> std::same_as<std::optional<jzh::JizhakError>>;
        { table.get_worker() } -> std::same_as<std::shared_ptr<jzh::BaseWorker>>;
        { table.operator->() } -> std::same_as<jzh::BaseWorker&>;
    };

    template <typename TWorker>
    concept is_supported_worker = requires(TWorker worker, jzh::Task task) {
        requires std::is_base_of_v<jzh::BaseWorker, TWorker>;
        requires std::default_initializable<TWorker>;

        { worker.start(std::function<void(std::stop_token)>()) } -> std::same_as<void>;
        { worker.add_task(task) } -> std::same_as<void>;
        { worker.notify() } -> std::same_as<void>;
        { worker.start_shutdown() } -> std::same_as<void>;
        { worker.join() } -> std::same_as<void>;
        { worker.request_stop() } -> std::same_as<void>;
        { worker.get_id() } -> std::same_as<std::jthread::id>;
    };
} // namespace jzh::concepts

export namespace jzh {
    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    class ThreadPoolManager : protected ThreadPoolManagerBase, public std::enable_shared_from_this<ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>> {
    public:
        friend class BaseWorker;

        using InfoTable = std::map<TInfoSorter, TInfoTable>;
        using OptionalError = std::optional<JizhakError>;

    private:
        InfoTable info_table_{};

        std::atomic<size_t> pending_tasks_{0};

        std::atomic<bool> pause_ = false;

        mutable std::mutex info_table_mutex_{};

        mutable std::mutex wait_mutex_;
        std::condition_variable wait_cv_;

    protected:
        std::jthread::id create_worker();
        std::expected<std::vector<std::jthread::id>, JizhakError> create_worker(unsigned int quantity = 1);
        OptionalError stop_worker(std::jthread::id thread_id);

        std::expected<std::weak_ptr<TWorker>, JizhakError> get_worker(std::jthread::id thread_id) override;
        std::expected<std::weak_ptr<TWorker>, JizhakError> get_worker_by_index(size_t index) override;

        std::expected<Task::id_t, JizhakError> create_task(Task& task, TaskInfo& task_info);
        OptionalError delete_task(Task::id_t task_id);
        OptionalError delete_task(Task::id_t task_id, std::jthread::id thread_id);

        OptionalError task_completed(Task::id_t task_id, std::jthread::id thread_id) override;
        OptionalError notify_steal_task(Task::id_t task_id,
                                        std::jthread::id to_thread_id,
                                        std::jthread::id from_thread_id) override;

        [[nodiscard]] size_t __number_workers() const override;

        [[nodiscard]] bool __is_paused() const override;

        std::expected<Task::id_t, JizhakError> __add_task(Task& task, TaskInfo& task_info) override;

    public:
        explicit ThreadPoolManager() = default;
        explicit ThreadPoolManager(unsigned int quantity);

        ThreadPoolManager(const ThreadPoolManager&) = delete;
        ThreadPoolManager(ThreadPoolManager&&) = default;

        ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
        ThreadPoolManager& operator=(ThreadPoolManager&&) = default;

        ~ThreadPoolManager() override;


        std::expected<const InfoTable&, JizhakError> operator[]() const;

        std::expected<const TInfoTable&, JizhakError> operator[](std::jthread::id thread_id) const;
        std::expected<const TaskInfo, JizhakError> operator[](Task::id_t task_id) const;


        std::expected<Task::id_t, JizhakError> operator()(Task& task, TaskInfo& task_info);

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(std::tuple<Task, TaskInfo> task_bundle);

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(F&& func, Args&&... args);

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(TaskInfo task_info, F&& func, Args&&... args);

        std::expected<std::jthread::id, JizhakError> add_worker();
        OptionalError remove_worker(std::jthread::id thread_id);

        std::expected<Task::id_t, JizhakError> add_task(Task& task, TaskInfo& task_info);

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(std::tuple<Task, TaskInfo> task_bundle);

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(F&& func, Args&&... args);

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(TaskInfo task_info, F&& func, Args&&... args);

        OptionalError remove_task(Task::id_t task_id); // beta
        OptionalError remove_task(Task::id_t task_id, std::jthread::id thread_id);

        void wait_all();
        template<class Rep, class Period>
        OptionalError wait_all(const std::chrono::duration<Rep, Period>& timeout);

        OptionalError stop_all();
        template<class Rep, class Period>
        OptionalError stop_all(const std::chrono::duration<Rep, Period>& timeout);

        void instant_stop_all();

        void pause();
        void resume();

        void notify_all();
        void notify(std::jthread::id thread_id);

        std::expected<const InfoTable&, JizhakError> get_info_table() const;

        std::expected<const TInfoTable&, JizhakError> search_worker_for(std::jthread::id thread_id) const;
        std::expected<const TaskInfo, JizhakError> search_task_for(Task::id_t task_id) const;

        [[nodiscard]] size_t size() const;
        [[nodiscard]] size_t number_tasks() const;
        [[nodiscard]] size_t number_workers() const;

        [[nodiscard]] bool is_there_task() const;
        [[nodiscard]] bool is_paused() const;
    };
} // namespace jzh