export module jizhak.thread_pool.this_thread;

import jizhak.thread_pool.tpm_base;
import jizhak.thread_pool.task;
import std;


namespace jzh::this_thread {
    inline thread_local std::weak_ptr<ThreadPoolManagerBase> current_tpm{};
}

export namespace jzh::this_thread {
    inline void set_tpm(const std::weak_ptr<ThreadPoolManagerBase>& tpm) {
        current_tpm = tpm;
    }

    [[nodiscard]] inline std::weak_ptr<ThreadPoolManagerBase> get_tpm() {
        return current_tpm;
    }

    [[nodiscard]] inline bool is_multithreaded() {
        return current_tpm.lock() != nullptr;
    }

    inline TaskPointer add_task(TaskPointer& task) {
        if (const auto tpm = get_tpm().lock())
            if (auto result = tpm->add_task(task); result.has_value())
                return task;
            else
                throw result.error();

        if (auto result = task(); result.has_value())
            throw result.value();

        task->task_info.status = TaskStatus::completed_synchronously;
        return task;
    }

    template <typename F, typename... Args>
    inline std::pair<std::future<std::invoke_result_t<F, Args...>>, TaskPointer>
    add_task(const TaskInfo task_info, F&& func, Args&&... args) {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto bound_func = std::bind_front(std::forward<F>(func), std::forward<Args>(args)...);
        std::packaged_task<ReturnType()> packaged_task(std::move(bound_func));

        std::future<ReturnType> future_result = packaged_task.get_future();
        Task::Function work_function          = [pt = std::move(packaged_task)]() mutable {
            pt();
        };

        auto task = make_task(std::move(work_function), task_info);

        return std::make_pair(std::move(future_result), add_task(task));
    }

    template <typename F, typename... Args>
    requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
    inline std::pair<std::future<std::invoke_result_t<F, Args...>>, TaskPointer>
    add_task(const TaskInfoField task_info_field, F&& func, Args&&... args) {
        return add_task(TaskInfo(task_info_field), std::forward<F>(func), std::forward<Args>(args)...);
    }
    template <typename F, typename... Args>
    requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
    inline std::pair<std::future<std::invoke_result_t<F, Args...>>, TaskPointer>
    add_task(F&& func, Args&&... args) {
        return add_task(TaskInfo(), std::forward<F>(func), std::forward<Args>(args)...);
    }
} // jzh::this_thread