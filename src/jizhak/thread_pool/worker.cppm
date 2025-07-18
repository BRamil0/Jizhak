export module jizhak.thread_pool.worker;

export import jizhak.thread_pool.task;
import std;

namespace jzh {
    class ThreadPoolManager;
} // namespace jzh

export namespace jzh {
    class Worker {
    private:
        std::jthread thread;
        std::jthread::id id;

        std::deque<Task> tasks{};
        mutable std::mutex queue_mutex{};
        std::condition_variable cv{};

        std::weak_ptr<ThreadPoolManager> tpm{};

    protected:
        void run_loop(std::stop_token token);

    public:
        Worker() = delete;
        Worker(const Worker&) = delete;
        Worker(Worker&&) = delete;

        Worker& operator=(const Worker&) = delete;
        Worker& operator=(Worker&&) = delete;

        ~Worker() = default;

        explicit Worker(const std::weak_ptr<ThreadPoolManager> &new_tpm) : tpm(new_tpm) {}

        void start();

        void add_task(Task new_task);

        [[nodiscard]] size_t size() const;

        [[nodiscard]] bool empty() const;
    };

    struct WorkerStats {
        std::atomic<size_t> total_tasks = 0;
        std::atomic<size_t> async_tasks = 0;
    };
} // namespace jzh