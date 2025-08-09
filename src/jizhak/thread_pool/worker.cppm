module;

#if !defined(USE_OF_STD_MODULE)
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <unordered_map>
#endif

export module jizhak.thread_pool.worker;

export import jizhak.thread_pool.task;
export import jizhak.error;

#if defined(USE_OF_STD_MODULE)
import std;
#endif


export namespace jzh {
    class BaseWorker {
    public:
        using OptionalError = std::optional<JizhakError>;

    private:
        std::jthread thread{};
        std::jthread::id id{};

        std::condition_variable cv{};
        std::atomic<bool> is_shutdown = false;
    protected:
        std::deque<TaskPointer> tasks{};
        mutable std::mutex queue_mutex{};

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

        std::optional<JizhakError> remove_task(Task::id_t task_id);

        void run_loop(std::stop_token token);

        void notify();

        void start_shutdown();

        void join();

        void instant_stop();

        virtual std::optional<std::deque<TaskPointer>> yield_half_of_tasks();

        [[nodiscard]] std::size_t size() const;

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

    struct WorkerInfo {
        std::jthread::id id{};
        std::size_t total_tasks = 0;
        std::size_t async_tasks = 0;
        bool is_shutting_down = false;
        std::unordered_map<Task::id_t, TaskPointer> tasks{};
    };
} // namespace jzh