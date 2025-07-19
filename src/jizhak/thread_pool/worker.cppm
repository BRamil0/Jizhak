export module jizhak.thread_pool.worker;

export import jizhak.thread_pool.task;
import std;

export namespace jzh {
    class Worker {
    private:
        std::jthread thread;
        std::jthread::id id;

        std::deque<Task> tasks{};
        mutable std::mutex queue_mutex{};
        std::condition_variable cv{};
    protected:
        void run_loop(std::stop_token token);

    public:
        Worker() = default;
        Worker(const Worker&) = delete;
        Worker(Worker&&) = delete;

        Worker& operator=(const Worker&) = delete;
        Worker& operator=(Worker&&) = delete;

        virtual ~Worker() = default;

        void start(std::function<void(std::stop_token)> work_function);

        void add_task(Task new_task);

        [[nodiscard]] size_t size() const;

        [[nodiscard]] bool empty() const;
    };
} // namespace jzh