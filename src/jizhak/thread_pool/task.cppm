export module jizhak.thread_pool.task;
import std;
import jizhak.error;

export namespace jzh {
    struct Task {
        using Function = std::function<void()>;

        Function function = nullptr;

        std::chrono::seconds time_out = std::chrono::seconds::zero();
        long long priority = 0;

        Task() = default;

        explicit Task(Function new_func, std::chrono::seconds time_out = std::chrono::seconds(0), long long priority = 0)
            : function(std::move(new_func)), time_out(time_out), priority(priority) {}

        std::optional<JizhakError> operator()() { // NOLINT(readability-make-member-function-const)
            if (function) function();
            else return JizhakError{JizhakErrorID::function_is_empty};
            return std::nullopt;
        }
    };
} // namespace jzh