export module jizhak.thread_pool.worker;

export import jizhak.thread_pool.task;
import std;

export namespace jzh {
    class ThreadPoolManager;

    class Worker {
    private:
        std::jthread thread;
        std::jthread::id id;

        std::deque<Task> tasks{};
        mutable std::mutex queue_mutex{};
        std::condition_variable cv{};

        std::weak_ptr<ThreadPoolManager> tpm{};

        void run_loop(std::stop_token token) {
            while (!token.stop_requested()) {
                Task task_to_run;
                {
                    std::unique_lock lock(queue_mutex);
                    cv.wait(lock, [this, &token] { return !tasks.empty() || token.stop_requested(); });

                    if (token.stop_requested()) return;

                    task_to_run = std::move(tasks.front());
                    tasks.pop_front();
                }

                if (task_to_run.function) {
                    task_to_run();
                }
            }
        }
    public:
        Worker() = delete;
        Worker(const Worker&) = delete;
        Worker(Worker&&) = delete;

        Worker& operator=(const Worker&) = delete;
        Worker& operator=(Worker&&) = delete;

        ~Worker() = default;

        explicit Worker(const std::weak_ptr<ThreadPoolManager> &new_tpm) : tpm(new_tpm) {}

        void start() {
            this->thread = std::jthread(&Worker::run_loop, this);
            this->id = thread.get_id();
        }

        void add_task(Task new_task) {
            {
                std::scoped_lock lock(queue_mutex);
                tasks.push_back(std::move(new_task));
            }
            cv.notify_one();
        }

        [[nodiscard]] size_t size() const {
            std::scoped_lock lock(queue_mutex);
            return tasks.size();
        }

        [[nodiscard]] bool empty() const {
            std::scoped_lock lock(queue_mutex);
            return tasks.empty();
        }
    };
} // namespace jzh