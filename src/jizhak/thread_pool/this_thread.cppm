export module jizhak.thread_pool.this_thread;

import jizhak.thread_pool.tpm_base;
import jizhak.thread_pool.task;
import std;


namespace jzh::this_thread {
    inline thread_local std::weak_ptr<ThreadPoolManagerBase> current_tpm{};
}

export namespace jzh::this_thread {
    /// **Тільки для jzh::ThreadPoolManager**
    inline void set_tpm(const std::weak_ptr<ThreadPoolManagerBase>& tpm) {
        current_tpm = tpm;
    }

    [[nodiscard]] inline std::weak_ptr<ThreadPoolManagerBase> get_tpm() {
        return current_tpm;
    }

    inline std::expected<Task::id_t, JizhakError> add_task(TaskPointer& task) {
        if (const auto tpm = get_tpm().lock())
            return tpm->add_task(task);
        task();
        return std::unexpected<JizhakError>(JizhakErrorID::failed_start_in_stream);
    }
    template <typename F, typename... Args>
    inline std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    add_task(const TaskInfo task_info, F&& func, Args&&... args) {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto bound_func = std::bind_front(std::forward<F>(func), std::forward<Args>(args)...);
        std::packaged_task<ReturnType()> packaged_task(std::move(bound_func));

        std::future<ReturnType> future_result = packaged_task.get_future();
        Task::Function work_function          = [pt = std::move(packaged_task)]() mutable {
            pt();
        };

        auto task = make_task(std::move(work_function), task_info);

        if (auto result = add_task(task); result.has_value())
            return std::make_tuple(std::move(future_result), *result);
        else
            return std::unexpected(result.error());
    }

    template <typename F, typename... Args>
    requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
    inline std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    add_task(const TaskInfoField task_info_field, F&& func, Args&&... args) {
        return add_task(TaskInfo(task_info_field), std::forward<F>(func), std::forward<Args>(args)...);
    }
    template <typename F, typename... Args>
    requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
    inline std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    add_task(F&& func, Args&&... args) {
        return add_task(TaskInfo(), std::forward<F>(func), std::forward<Args>(args)...);
    }
} // jzh::this_thread