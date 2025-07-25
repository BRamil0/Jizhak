export module jizhak.thread_pool.task;
import std;
import jizhak.error;

export namespace jzh {
    struct Task;
    struct TaskInfo;

    enum struct TaskStatus {
        completed,
        completed_with_error,
        error,
        in_progress,
        waiting,
        none,
    };

    struct TaskInfo {
        using id_t = unsigned long long;

        inline static constinit std::atomic<id_t> global_task_id = 0;

        id_t id{};
        std::string hash_function{};

        TaskStatus status = TaskStatus::none;

        std::chrono::seconds timeout = std::chrono::seconds::zero();
        long long priority = 0;
        std::jthread::id worker_id{};

        bool is_task_completed = false;
        bool is_async = false;

        static id_t get_new_id() {
            return global_task_id++;
        }

        TaskInfo() : id(get_new_id()) {};
        explicit TaskInfo(const std::chrono::seconds timeout, const long long priority, const bool is_async)
            : id(get_new_id()), timeout(timeout), priority(priority), is_async(is_async) {}
    };

    struct Task {
        using Function = std::move_only_function<void() noexcept(false)>;
        using id_t = TaskInfo::id_t;

        Function function = nullptr;
        TaskInfo task_info{};

        Task() = default;

        explicit Task(Function new_func) : function(std::move(new_func)) {}
        explicit Task(Function new_func, const TaskInfo& task_info) : function(std::move(new_func)), task_info(task_info) {}
        explicit Task(const TaskInfo& task_info, Function new_func) : task_info(task_info), function(std::move(new_func)) {}

        std::optional<JizhakError> operator()() {
            if (function) function();
            else return JizhakError{JizhakErrorID::function_is_empty};
            return std::nullopt;
        }
    };

    struct TaskPointer {
        std::shared_ptr<Task> task_ptr{};

        TaskPointer() = default;
        explicit TaskPointer(std::shared_ptr<Task> task_ptr) : task_ptr(std::move(task_ptr)) {}

        std::optional<JizhakError> operator()() const {
            return task_ptr->operator()();
        }
    };


    TaskPointer make_task(Task::Function func) {
        return TaskPointer(std::make_shared<Task>(std::move(func)));
    }

    TaskPointer make_task(Task::Function func, TaskInfo task_info) {
        return TaskPointer(std::make_shared<Task>(std::move(func), std::move(task_info)));
    }

    TaskPointer make_task(TaskInfo task_info, Task::Function func) {
        return TaskPointer(std::make_shared<Task>(std::move(task_info), std::move(func)));
    }

    TaskPointer make_task(Task::Function func, const std::chrono::seconds timeout, const long long priority, const bool is_async) {
        return TaskPointer(std::make_shared<Task>(std::move(func), TaskInfo(timeout, priority, is_async)));
    }

} // namespace jzh