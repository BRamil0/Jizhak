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
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    class ThreadPoolManager : protected ThreadPoolManagerBase {
    public:
        friend class BaseWorker;

        using Workers = TContainer<std::unique_ptr<TWorker>>;
        using TableWorker = std::unordered_map<std::jthread::id, WorkerStats>;
        using TableTask = std::unordered_map<std::jthread::id, std::vector<TaskInfo>>;

        using OptionalError = std::optional<JizhakError>;

    private:
        Workers workers_{};

        std::atomic<size_t> pending_tasks_{0};

        mutable std::mutex wait_mutex_;
        std::condition_variable wait_cv_;

        TableWorker table_worker_stats_{};
        TableTask table_task_infos_{};

        bool pause_ = false;

    protected:
        std::expected<std::jthread::id, JizhakError> create_worker(unsigned int quantity = 1);
        OptionalError stop_worker(std::jthread::id thread_id); // beta

        std::expected<const TWorker*, JizhakError> get_worker(std::jthread::id thread_id);

        std::expected<Task::id_t, JizhakError> create_task(Task &task);
        OptionalError delete_task(Task::id_t task_id); // beta
        OptionalError delete_task(Task::id_t task_id, std::jthread::id thread_id); // beta

        OptionalError task_completed(Task::id_t task_id, std::jthread::id thread_id) override;
        OptionalError notify_steal_task(Task::id_t task_id,
                                        std::jthread::id to_thread_id,
                                        std::jthread::id from_thread_id) override;

        TWorker* get_worker_by_index(size_t index) override;

        size_t __number_workers() override;

    public:
        explicit ThreadPoolManager() = default;
        explicit ThreadPoolManager(unsigned int quantity);

        ThreadPoolManager(const ThreadPoolManager&) = delete;
        ThreadPoolManager(ThreadPoolManager&&) = default;

        ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
        ThreadPoolManager& operator=(ThreadPoolManager&&) = default;

        ~ThreadPoolManager() override;


        std::expected<const WorkerStats&, JizhakError> operator[](std::jthread::id thread_id) const;
        std::expected<const TaskInfo&, JizhakError> operator[](Task::id_t task_id) const;


        template <typename F, typename... Args> // Метод для додавання функцій.
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(F&& func, Args&&... args);

        std::expected<std::jthread::id, JizhakError> add_worker(unsigned int quantity = 1);
        OptionalError remove_worker(unsigned int quantity = 1); // beta
        OptionalError remove_worker(std::jthread::id thread_id); // beta

        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(F&& func, Args&&... args);

        OptionalError remove_task(Task::id_t task_id); // beta
        OptionalError remove_task(Task::id_t task_id, std::jthread::id thread_id); // beta

        OptionalError wait_all();

        template<class Rep, class Period>
        OptionalError wait_all(const std::chrono::duration<Rep, Period>& time_out);

        OptionalError stop_all();

        template<class Rep, class Period>
        OptionalError stop_all(const std::chrono::duration<Rep, Period>& time_out);

        OptionalError pause();
        OptionalError resume();

        std::expected<const TableWorker&, JizhakError> get_table_worker();
        std::expected<const TableTask&, JizhakError> get_table_task();

        std::expected<const WorkerStats&, JizhakError> search_worker_for(std::jthread::id thread_id) const;
        std::expected<const TaskInfo&, JizhakError> search_task_for(Task::id_t task_id) const;

        size_t size() const;
        size_t number_tasks() const;
        size_t number_workers() const;

        [[nodiscard]] bool is_there_task() const;
        [[nodiscard]] bool is_paused() const;
    };
} // namespace jzh