export module jizhak.thread_pool.worker;

export import jizhak.thread_pool.task;
export import jizhak.error;

import std;

export namespace jzh {
    class BaseWorker {
    public:
        using OptionalError = std::optional<JizhakError>;
    private:
        std::jthread thread{};
        std::jthread::id id{};

        std::condition_variable cv{};
    protected:
        std::deque<Task> tasks{};
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

        void add_task(Task new_task);

        virtual std::optional<std::deque<Task>> yield_half_of_tasks();

        [[nodiscard]] size_t size() const;

        [[nodiscard]] bool empty() const;

        [[nodiscard]] std::jthread::id get_id() const;

    };
} // namespace jzh