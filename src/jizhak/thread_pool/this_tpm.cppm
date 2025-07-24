export module jizhak.thread_pool.this_tpm;

import jizhak.thread_pool.tpm_base;
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

    inline std::expected<Task::id_t, JizhakError> add_task(Task& task, TaskInfo& task_info) {
        if (const auto tpm = get_tpm().lock()) {
            return tpm->__add_task(task, task_info);
        }
        task();
        return std::unexpected<JizhakError>(JizhakErrorID::failed_start_in_stream);

    }

    template <typename F, typename ... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    add_task(std::tuple<Task, TaskInfo> task_bundle) {
        return std::apply(
           [](Task& task, TaskInfo& task_info) {
               return add_task(task, task_info);
           },
           task_bundle
       );
    }

    template <typename F, typename... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    Tadd_task(F&& func, Args&&... args) {
        return add_task(task_info_set_id(TaskInfo()), std::forward<F>(func), std::forward<Args>(args)...);
    }

    template <typename F, typename ... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    add_task(TaskInfo task_info, F&& func, Args&&... args) {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto bound_func = std::bind_front(std::forward<F>(func), std::forward<Args>(args)...);
        std::packaged_task<ReturnType()> packaged_task(std::move(bound_func));

        std::future<ReturnType> future_result = packaged_task.get_future();
        Task::Function work_function = [pt = std::move(packaged_task)]() mutable {
            pt();
        };

        auto [task, new_task_info] = make_task(std::move(work_function), task_info);

        if (auto creation_result = add_task(task, new_task_info); !creation_result)
            return std::unexpected(creation_result.error());

        return std::make_tuple(std::move(future_result), new_task_info.id);
    }
}