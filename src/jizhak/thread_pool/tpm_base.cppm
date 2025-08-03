export module jizhak.thread_pool.tpm_base;

export import jizhak.error;

import jizhak.thread_pool.worker;

import jizhak.std;

export namespace jzh {
    class ThreadPoolManagerBase {
    public:
        friend class BaseWorker;
        friend class Worker;

    protected:
        virtual std::optional<JizhakError> _task_completed(Task::id_t task_id, std::jthread::id task_completed) = 0;

        virtual std::optional<JizhakError> _on_task_stolen(Task::id_t task_id,
                                      std::jthread::id to_thread_id,
                                      std::jthread::id from_thread_id) = 0;

    public:
        virtual ~ThreadPoolManagerBase() = default;

        virtual std::expected<Task::id_t, JizhakError> add_task(TaskPointer& task) = 0;

        virtual std::expected<Task::id_t, JizhakError> start_task(Task::id_t task_id) = 0;

        virtual void notify_all() = 0;

        virtual void notify(std::jthread::id thread_id) = 0;

        [[nodiscard]] virtual std::unordered_map<Task::id_t, TaskPointer> get_task_registry() const = 0;

        [[nodiscard]] virtual std::expected<std::map<std::jthread::id, WorkerInfo>, JizhakError> get_workers_info() const = 0;


        [[nodiscard]] virtual std::expected<WorkerInfo, JizhakError> find_worker_info(std::jthread::id thread_id) const = 0;


        [[nodiscard]] virtual std::expected<TaskPointer, JizhakError> find_task(Task::id_t task_id) const = 0;

        [[nodiscard]] virtual std::size_t size() const = 0;


        [[nodiscard]] virtual std::size_t number_tasks() const = 0;

        [[nodiscard]] virtual std::size_t number_workers() const = 0;


        [[nodiscard]] virtual bool is_there_task() const = 0;

        [[nodiscard]] virtual bool is_paused() const = 0;

        virtual std::optional<std::deque<TaskPointer>> steal_tasks_from(std::jthread::id victim_id) = 0;
    };
}// namespace jzh