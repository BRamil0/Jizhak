export module jizhak.thread_pool.task;
import std;
import jizhak.error;

export namespace jzh {
    struct TaskInfo;

    struct Task {
        using id_t = unsigned long long;
        using Function = std::function<void()>;

        id_t id{};
        Function function = nullptr;

        Task() = default;

        explicit Task(const id_t id, Function new_func)
            : id(id), function(std::move(new_func)) {}

        explicit Task(const TaskInfo& task_info, Function new_func);

        std::optional<JizhakError> operator()() { // NOLINT(readability-make-member-function-const)
            if (function) function();
            else return JizhakError{JizhakErrorID::function_is_empty};
            return std::nullopt;
        }
    };

    struct TaskInfo {
        Task::id_t id{};

        std::chrono::seconds timeout = std::chrono::seconds::zero();

        long long priority = 0;
        bool is_async = false;

        TaskInfo() = default;

        explicit TaskInfo(const Task::id_t id, const std::chrono::seconds timeout, const long long priority, const bool is_async)
            : id(id), timeout(timeout), priority(priority), is_async(is_async) {}

        explicit TaskInfo(const std::chrono::seconds timeout, const long long priority, const bool is_async)
            : timeout(timeout), priority(priority), is_async(is_async) {}

        explicit TaskInfo(const Task& task);
    };

    Task::Task(const TaskInfo& task_info, Function new_func): id(task_info.id), function(std::move(new_func)) {}
    TaskInfo::TaskInfo(const Task& task): id(task.id) {}

    static constinit std::atomic<Task::id_t> TASK_ID = 0;

    TaskInfo& task_info_set_id(TaskInfo& task_info) {
        const Task::id_t id = TASK_ID++;
        task_info.id = id;
        return task_info;
    }

    std::tuple<Task, TaskInfo> make_task(const Task::Function& func, const std::chrono::seconds timeout, const long long priority, const bool is_async) {
        const Task::id_t id = TASK_ID++;
        auto task = Task(id, func);
        auto task_info = TaskInfo(id, timeout, priority, is_async);
        return std::make_tuple(task, task_info);
    }

    std::tuple<Task, TaskInfo> make_task(const Task::Function& func, const TaskInfo& task_info) {
        return std::make_tuple(Task(task_info, func), task_info);
    }

    std::tuple<Task, TaskInfo> make_task(const Task::Function& func) {
        const Task::id_t id = TASK_ID++;
        auto task = Task(id, func);
        auto task_info = TaskInfo();
        return std::make_tuple(task, task_info);
    }

} // namespace jzh