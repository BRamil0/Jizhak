export module jizhak.thread_pool.tpm;
import std;

import jizhak.thread_pool.tpm_base;
export import jizhak.thread_pool.task;
export import jizhak.thread_pool.worker;
export import jizhak.error;

export template <template <typename...> typename TContainer, typename TElement>
concept is_supported_container = requires(TContainer<TElement> container, TElement element) {
    { container.push_back(std::move(element)) };

    { container.begin() } -> std::forward_iterator;
    { container.end() } -> std::forward_iterator;
};

export template <typename TWorker>
concept is_supported_worker = requires(TWorker worker, jzh::Task task) {
    requires std::default_initializable<TWorker>;

    { worker.start(std::function<void(std::stop_token)>()) } -> std::same_as<void>;
    { worker.add_task(task) } -> std::same_as<void>;
};

export namespace jzh {
    template <template <typename...> typename TContainer = std::vector, typename TWorker = Worker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    class ThreadPoolManager : protected ThreadPoolManagerBase, public std::enable_shared_from_this<ThreadPoolManager<TContainer, TWorker>> {
    public:
        friend class BaseWorker;

        using Workers = TContainer<std::shared_ptr<TWorker>>;
        using TableWorker = std::unordered_map<std::jthread::id, SynchronizedWorkerStats>;
        using TableTask = std::unordered_map<std::jthread::id, SynchronizedTaskInfos>;

        using OptionalError = std::optional<JizhakError>;

    private:
        Workers workers_{};
        std::unordered_map<std::jthread::id, std::weak_ptr<TWorker>> worker_for_id_{};
        std::atomic<size_t> pending_tasks_{0};

        TableWorker table_worker_stats_{};
        TableTask table_task_infos_{};

        std::atomic<bool> pause_ = false;

        mutable std::mutex workers_mutex_{};
        mutable std::mutex tables_mutex_{};

        mutable std::mutex wait_mutex_;
        std::condition_variable wait_cv_;

    protected:
        std::jthread::id create_worker();
        std::expected<std::vector<std::jthread::id>, JizhakError> create_worker(unsigned int quantity = 1);
        OptionalError stop_worker(std::jthread::id thread_id);

        std::expected<const TWorker*, JizhakError> get_worker(std::jthread::id thread_id);

        std::expected<Task::id_t, JizhakError> create_task(Task &task);
        OptionalError delete_task(Task::id_t task_id); // beta
        OptionalError delete_task(Task::id_t task_id, std::jthread::id thread_id); // beta

        OptionalError task_completed(Task::id_t task_id, std::jthread::id thread_id) override;
        OptionalError notify_steal_task(Task::id_t task_id,
                                        std::jthread::id to_thread_id,
                                        std::jthread::id from_thread_id) override;

        std::weak_ptr<TWorker> get_worker_by_index(size_t index) override;

        [[nodiscard]] size_t __number_workers() const override;

        [[nodiscard]] bool __is_paused() const override;

    public:
        explicit ThreadPoolManager() = default;
        explicit ThreadPoolManager(unsigned int quantity);

        ThreadPoolManager(const ThreadPoolManager&) = delete;
        ThreadPoolManager(ThreadPoolManager&&) = default;

        ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
        ThreadPoolManager& operator=(ThreadPoolManager&&) = default;

        ~ThreadPoolManager() override;


        std::expected<const WorkerStats&, JizhakError> operator[](std::jthread::id thread_id) const;
        std::expected<const TaskInfo, JizhakError> operator[](Task::id_t task_id) const;


        template <typename F, typename... Args> // Метод для додавання функцій.
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(F&& func, Args&&... args);

        std::expected<std::jthread::id, JizhakError> add_worker();
        OptionalError remove_worker(std::jthread::id thread_id);

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(F&& func, Args&&... args);

        OptionalError remove_task(Task::id_t task_id); // beta
        OptionalError remove_task(Task::id_t task_id, std::jthread::id thread_id); // beta

        void wait_all();
        template<class Rep, class Period>
        OptionalError wait_all(const std::chrono::duration<Rep, Period>& timeout);

        OptionalError stop_all();
        template<class Rep, class Period>
        OptionalError stop_all(const std::chrono::duration<Rep, Period>& timeout);

        void pause();
        void resume();

        void notify_all();
        void notify(std::jthread::id thread_id);

        std::expected<const TableWorker, JizhakError> get_table_worker() const;
        std::expected<const TableTask, JizhakError> get_table_task() const;

        std::expected<const WorkerStats&, JizhakError> search_worker_for(std::jthread::id thread_id) const;
        std::expected<const TaskInfo, JizhakError> search_task_for(Task::id_t task_id) const;

        [[nodiscard]] size_t size() const;
        [[nodiscard]] size_t number_tasks() const;
        [[nodiscard]] size_t number_workers() const;

        [[nodiscard]] bool is_there_task() const;
        [[nodiscard]] bool is_paused() const;
    };
} // namespace jzh