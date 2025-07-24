export module jizhak.thread_pool.tpm_base;

export import jizhak.error;

import jizhak.thread_pool.worker;

import std;

export namespace jzh {
    class ThreadPoolManagerBase {
    public:
        virtual ~ThreadPoolManagerBase() = default;

        virtual std::expected<std::weak_ptr<BaseWorker>, JizhakError> get_worker(std::jthread::id thread_id) = 0;
        virtual std::weak_ptr<BaseWorker> get_worker_by_index(size_t index) = 0;

        virtual std::optional<JizhakError> task_completed(Task::id_t task_id, std::jthread::id thread_id) = 0;
        virtual std::optional<JizhakError> notify_steal_task(Task::id_t, std::jthread::id to_thread_id, std::jthread::id from_thread_id) = 0;

        [[nodiscard]] virtual size_t __number_workers() const = 0;

        [[nodiscard]] virtual bool __is_paused() const = 0;

        virtual std::expected<Task::id_t, JizhakError> __add_task(Task &task) = 0;
    };
}// namespace jzh