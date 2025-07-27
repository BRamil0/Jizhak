export module jizhak.thread_pool.worker;

export import jizhak.thread_pool.task;
export import jizhak.error;

import std;

export namespace jzh {
    class BaseWorker {
    public:
        template <typename TInfoTable, typename TInfoSorter, typename TWorker>
        friend class ThreadPoolManager;

        using OptionalError = std::optional<JizhakError>;

    private:
        std::jthread thread{};
        std::jthread::id id{};

        std::condition_variable cv{};
        std::atomic<bool> is_shutdown = false;
    protected:
        std::deque<TaskPointer> tasks{};
        mutable std::mutex queue_mutex{};

        void run_loop(std::stop_token token);

        virtual OptionalError steal_task();

    public:
        BaseWorker() = default;
        BaseWorker(const BaseWorker&) = delete;
        BaseWorker(BaseWorker&&) = delete;

        BaseWorker& operator=(const BaseWorker&) = delete;
        BaseWorker& operator=(BaseWorker&&) = delete;

        virtual ~BaseWorker() = default;

        void start(std::function<void(std::stop_token)> work_function);

        void add_task(TaskPointer new_task);

        void notify();

        void start_shutdown();

        void join();

        void instant_stop();

        virtual std::optional<std::deque<TaskPointer>> yield_half_of_tasks();

        [[nodiscard]] size_t size() const;

        [[nodiscard]] bool empty() const;

        [[nodiscard]] std::jthread::id get_id() const;

    };

    class Worker : public BaseWorker {
    private:
        mutable std::mt19937 random_generator_{};

    public:
        OptionalError steal_task() override;
        explicit Worker() : BaseWorker(), random_generator_(std::random_device{}()) {}

    };
} // namespace jzh