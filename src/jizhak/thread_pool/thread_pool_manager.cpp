module jizhak.thread_pool.tpm;
import std;

import jizhak.thread_pool.task;
import jizhak.thread_pool.worker;

namespace jzh {
    // ThreadPoolManager: protected
    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<std::jthread::id, JizhakError> ThreadPoolManager<TContainer, TWorker>::create_worker(unsigned int quantity) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::stop_worker(std::jthread::id thread_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const TWorker*, JizhakError> ThreadPoolManager<TContainer, TWorker>::get_worker(std::jthread::id thread_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<Task::id_t, JizhakError> ThreadPoolManager<TContainer, TWorker>::create_task(Task& task) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::delete_task(Task::id_t task_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::delete_task(Task::id_t task_id, std::jthread::id thread_id) {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::task_completed(Task::id_t task_id, std::jthread::id thread_id) override {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::notify_steal_task(Task::id_t task_id,
        std::jthread::id to_thread_id, std::jthread::id from_thread_id) override {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    TWorker* ThreadPoolManager<TContainer, TWorker>::get_worker_by_index(size_t index) override {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::__number_workers() override {}

    // ThreadPoolManager: public
    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::ThreadPoolManager(unsigned int quantity) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::~ThreadPoolManager() override {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const WorkerStats&, JizhakError> ThreadPoolManager<TContainer, TWorker>::operator[](std::jthread::id thread_id) const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const TaskInfo&, JizhakError> ThreadPoolManager<TContainer, TWorker>::operator[](Task::id_t task_id) const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    template <typename F, typename... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::operator()(F&& func, Args&&... args) {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<std::jthread::id, JizhakError> ThreadPoolManager<TContainer, TWorker>::add_worker(unsigned int quantity) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::remove_worker(unsigned int quantity) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::remove_worker(std::jthread::id thread_id) {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    template <typename F, typename... Args> std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::add_task(F&& func, Args&&... args) {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::remove_task(Task::id_t task_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::remove_task(Task::id_t task_id, std::jthread::id thread_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::wait_all() {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    template <class Rep, class Period>
    ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::wait_all(const std::chrono::duration<Rep, Period>& time_out) {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::pause() {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::resume() {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const typename ThreadPoolManager<TContainer, TWorker>::TableWorker&, JizhakError> ThreadPoolManager<TContainer, TWorker>::get_table_worker() {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const typename ThreadPoolManager<TContainer, TWorker>::TableTask&, JizhakError> ThreadPoolManager<TContainer, TWorker>::get_table_task() {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const WorkerStats&, JizhakError> ThreadPoolManager<TContainer, TWorker>::search_worker_for(std::jthread::id thread_id) const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const TaskInfo&, JizhakError> ThreadPoolManager<TContainer, TWorker>::search_task_for(Task::id_t task_id) const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::size() const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::number_tasks() const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::number_workers() const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    [[nodiscard]] bool ThreadPoolManager<TContainer, TWorker>::is_there_task() const {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::unique_ptr<TWorker>> && is_supported_worker<TWorker>)
    [[nodiscard]] bool ThreadPoolManager<TContainer, TWorker>::is_paused() const {}

} // namespace jzh
