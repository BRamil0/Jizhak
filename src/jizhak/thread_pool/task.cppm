export module jizhak.thread_pool.task;
import std;
import jizhak.error;

export namespace jzh {
    struct Task {
        using Function = std::function<void()>;

        unsigned long long id{};
        Function function = nullptr;

        std::chrono::seconds time_out = std::chrono::seconds::zero();
        long long priority = 0;
        bool is_async = false;

        Task() = default;

        explicit Task(unsigned long long id, Function new_func, bool is_async = false, std::chrono::seconds time_out = std::chrono::seconds(0), long long priority = 0)
            : id(id), function(std::move(new_func)), time_out(time_out), priority(priority), is_async(is_async) {}

        std::optional<JizhakError> operator()() { // NOLINT(readability-make-member-function-const)
            if (function) function();
            else return JizhakError{JizhakErrorID::function_is_empty};
            return std::nullopt;
        }
    };

    struct TaskInfo {
        unsigned long long id{};
        bool is_async{};
        int priority{};
        std::chrono::seconds time_out{};
    };

    struct WorkerStats {
        std::atomic<size_t> total_tasks = 0;
        std::atomic<size_t> async_tasks = 0;
    };
} // namespace jzh