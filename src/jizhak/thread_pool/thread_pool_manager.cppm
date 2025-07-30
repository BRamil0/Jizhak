export module jizhak.thread_pool.tpm;
import std;

import jizhak.thread_pool.tpm_base;
export import jizhak.thread_pool.task;
export import jizhak.thread_pool.worker;
export import jizhak_.thread_pool_.utilities_tpm;
export import jizhak.thread_pool.this_thread;
export import jizhak.error;

export namespace jzh::concepts {
    template <typename TInfoWorkerTable, typename TWorker> concept is_supported_info_table = requires(
    TInfoWorkerTable table, TaskPointer task, std::shared_ptr<TWorker> worker_ptr) {
        { TInfoWorkerTable(worker_ptr) };
        { table.add_task(task) } -> std::same_as<void>;
        { table.remove_task(task->task_info.id) } -> std::same_as<std::optional<JizhakError>>;
        { table.get_worker() } -> std::same_as<std::shared_ptr<BaseWorker>>;
        { table.operator->() } -> std::same_as<BaseWorker&>;
        };

    template <typename TWorker> concept is_supported_worker = requires(TWorker worker, TaskPointer task) {
        requires std::is_base_of_v<BaseWorker, TWorker>;
        requires std::default_initializable<TWorker>;
        { worker.start(std::function<void(std::stop_token)>()) } -> std::same_as<void>;
        { worker.add_task(task) } -> std::same_as<void>;
        { worker.notify() } -> std::same_as<void>;
        { worker.start_shutdown() } -> std::same_as<void>;
        { worker.join() } -> std::same_as<void>;
        { worker.instant_stop() } -> std::same_as<void>;
        { worker.get_id() } -> std::same_as<std::jthread::id>;
    };
} // namespace jzh::concepts

export namespace jzh {
    template <typename TWorkerRegistry     = WorkerRegistry<>,
              typename TTaskRegistry       = TaskRegistry,
              typename TWorker             = Worker>
    requires (concepts::is_supported_worker<TWorker>)
    class ThreadPoolManager : public ThreadPoolManagerBase,
                              public std::enable_shared_from_this<ThreadPoolManager<TWorkerRegistry, TTaskRegistry, TWorker>> {
    public:
        friend class BaseWorker;
        using OptionalError = std::optional<JizhakError>;

        using TWorkerRegistry_T     = TWorkerRegistry;
        using TTaskRegistry_T       = TTaskRegistry;
        using TWorker_T             = TWorker;

    private:
        TWorkerRegistry worker_registry_{};
        TTaskRegistry task_registry_{};

        std::atomic<size_t> pending_tasks_{0};

        std::atomic<bool> pause_ = false;
        std::atomic<bool> this_context_ = false;

        mutable std::mutex mutex_{};

        mutable std::mutex wait_mutex_;
        std::condition_variable wait_cv_;

    protected:
        std::jthread::id __create_worker() {
            auto worker_ptr = std::make_shared<TWorker>();

            auto work_function = [this, worker_raw = worker_ptr.get()](std::stop_token token) {
                this_thread::set_tpm(std::static_pointer_cast<ThreadPoolManagerBase>(this->shared_from_this()));
                worker_raw->run_loop(token);
            };
            worker_ptr->start(std::move(work_function));

            return this->worker_registry_.register_worker(
                std::move(typename TWorkerRegistry::TWorkerInfoTableSorter_T{ .id = worker_ptr->get_id() }),
                std::move(typename TWorkerRegistry::TTWorkerInfoTable_T(worker_ptr))
                );
        }

        std::expected<std::vector<std::jthread::id>, JizhakError> __create_worker(unsigned int quantity) {
            if (quantity == 0) return std::unexpected<JizhakError>(JizhakErrorID::zero_transferred);

            std::vector<std::jthread::id> ids{};
            ids.reserve(quantity);

            for ([[maybe_unused]] auto _ : std::ranges::iota_view{0u, quantity})
                ids.push_back(this->__create_worker());
            return ids;
        }

        OptionalError __stop_worker(const std::jthread::id thread_id) {
            auto worker_ptr = worker_registry_.shutdown_worker(thread_id);
            if (!worker_ptr)
                return JizhakError(JizhakErrorID::worker_not_found);

            worker_ptr.value()->start_shutdown();
            worker_ptr.value()->join();
            worker_registry_.final_unregister(thread_id);

            return std::nullopt;
        }

        Task::id_t __add_task(TaskPointer& task) {
            [[maybe_unused]] auto _ = task_registry_.add_task(task);
            return task->task_info.id;
        }

        OptionalError __remove_task(Task::id_t task_id) {
            return task_registry_.remove_task(task_id);
        }

        std::expected<Task::id_t, JizhakError> __start_task(Task::id_t task_id) {
            TaskPointer task;
            if (auto it = task_registry_.find_task(task_id); it.has_value())
                task = it.value();
            else
                return std::unexpected<JizhakError>(JizhakErrorID::task_not_found);

            if (auto it = worker_registry_.add_task(task); it.has_value()) {
                ++pending_tasks_;
                return it.value();
            } else
                return std::unexpected<JizhakError>(it.error());
        }

        OptionalError __stop_task(Task::id_t task_id) {
            if (auto result = worker_registry_.remove_task(task_id); result.has_value())
                return result.value();
            if (pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1)
                wait_cv_.notify_all();
            return std::nullopt;
        }

        OptionalError _task_completed(Task::id_t task_id, std::jthread::id task_completed) override {
            if (auto it = worker_registry_.on_task_completed(task_id, task_completed); it.has_value())
                return it.value();
            if (pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1)
                wait_cv_.notify_all();
            return std::nullopt;
        }

        std::expected<const TWorkerRegistry&, JizhakError> _get_worker_registry() const {
            return this->worker_registry_;
        }

        OptionalError _on_task_stolen(Task::id_t task_id,
                                      std::jthread::id to_thread_id,
                                      std::jthread::id from_thread_id) override {
            return worker_registry_.transfer_task_between_workers(task_id, from_thread_id, to_thread_id);
        }

    public:
        explicit ThreadPoolManager() = default;

        explicit ThreadPoolManager(unsigned int quantity) {
            if (auto result = this->add_worker(quantity); !result)
                throw std::runtime_error("Failed to create initial workers: " + std::string(result.error().what()));
        }

        ThreadPoolManager(const ThreadPoolManager&) = delete;
        ThreadPoolManager(ThreadPoolManager&&)      = default;

        ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
        ThreadPoolManager& operator=(ThreadPoolManager&&)      = default;

            ~ThreadPoolManager() override {
                try {
                    this->stop_all(std::chrono::minutes(10));
                    unregister_this_thread();
                }
                catch (...) {
                    this->instant_stop_all();
                }
            }

        OptionalError register_this_thread() {
            if (this_thread::get_tpm().lock() != nullptr)
                return JizhakError(JizhakErrorID::thread_already_registered);
            this_thread::set_tpm(std::static_pointer_cast<ThreadPoolManagerBase>(this->shared_from_this()));
            this_context_ = true;
            return std::nullopt;
        }

        OptionalError unregister_this_thread() {
            if (!this_context_.load() or this_thread::get_tpm().lock() == nullptr)
                return JizhakError(JizhakErrorID::the_established_thread_is_not_this_tpm);
            this_context_ = false;
            this_thread::unregister_this_thread();
            return std::nullopt;
        }

        std::expected<std::weak_ptr<BaseWorker>, JizhakError> get_worker(std::jthread::id thread_id) {
            std::scoped_lock lock(mutex_);
            if (auto it = worker_registry_.find(typename TWorkerRegistry::TWorkerTableSorter{ .id = thread_id }); it != worker_registry_.end()) {
                return it->second.get_worker();
            }
            return std::unexpected<JizhakError>(JizhakErrorID::worker_not_found);
        }


        std::expected<std::jthread::id, JizhakError> add_worker() {
            std::scoped_lock lock(mutex_);
            return this->__create_worker();
        }
        std::expected<std::vector<std::jthread::id>, JizhakError> add_worker(unsigned int quantity) {
            std::scoped_lock lock(mutex_);
            return this->__create_worker(quantity);
        }

        OptionalError remove_worker(const std::jthread::id thread_id) {
            return this->__stop_worker(thread_id);
        }


        std::expected<Task::id_t, JizhakError> add_task(TaskPointer& task) override {
            std::scoped_lock lock(mutex_);
            Task::id_t task_id = this->__add_task(task);
            if (auto it = this->__start_task(task_id); it.has_value())
                return it.value();
            else {
                __remove_task(task_id);
                return std::unexpected<JizhakError>(it.error());
            }
        }
        template <typename F, typename... Args>
        std::expected<std::pair<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(const TaskInfo task_info, F&& func, Args&&... args) {
            using ReturnType = std::invoke_result_t<F, Args...>;

            auto bound_func = std::bind_front(std::forward<F>(func), std::forward<Args>(args)...);
            std::packaged_task<ReturnType()> packaged_task(std::move(bound_func));

            std::future<ReturnType> future_result = packaged_task.get_future();
            Task::Function work_function          = [pt = std::move(packaged_task)]() mutable {
                pt();
            };

            auto task = make_task(std::move(work_function), task_info);

            std::scoped_lock lock(mutex_);

            auto task_id = this->__add_task(task);

            if (auto creation_result = __start_task(task_id); !creation_result)
                return std::unexpected(creation_result.error());

            return std::make_pair(std::move(future_result), task_id);
        }
        template <typename F, typename... Args>
        std::expected<std::pair<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(const TaskInfoField task_info_field, F&& func, Args&&... args) {
            return this->add_task(TaskInfo(task_info_field), std::forward<F>(func), std::forward<Args>(args)...);
        }
        template <typename F, typename... Args>
        requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
        std::expected<std::pair<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        add_task(F&& func, Args&&... args) {
            return this->add_task(TaskInfo(), std::forward<F>(func), std::forward<Args>(args)...);
        }

        OptionalError remove_task(const Task::id_t task_id) {
            std::scoped_lock lock(mutex_);

            if (auto it = this->__stop_task(task_id); it.has_value())
                return it.value();

            if (auto it = this->__remove_task(task_id); it.has_value())
                return it.value();

            return std::nullopt;
        }

        std::expected<Task::id_t, JizhakError> start_task(Task::id_t task_id) override {
            return this->__start_task(task_id);
        }

        OptionalError stop_task(const Task::id_t task_id) {
            return __stop_task(task_id);
        }


        void wait_all() {
            std::unique_lock lock(wait_mutex_);

            wait_cv_.wait(lock, [this] {
                return pending_tasks_.load() == 0;
            });
        }
        template <class Rep, class Period>
        OptionalError wait_all(const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock lock(wait_mutex_);

            const bool success = wait_cv_.wait_for(lock, timeout, [this] {
                return pending_tasks_.load() == 0;
            });

            if (success) return std::nullopt;
            return JizhakError{JizhakErrorID::timeout_expired};
        }


        OptionalError stop_all() {
            wait_all();

            auto result = worker_registry_.get_all_worker_ids();

            if (!result.has_value())
                return result.error();

            auto worker_ids_to_stop = result.value();

            for (const auto& id : worker_ids_to_stop) {
                __stop_worker(id);
            }

            return std::nullopt;
        }
        template <class Rep, class Period>
        OptionalError stop_all(const std::chrono::duration<Rep, Period>& timeout) {
            auto future = std::async(std::launch::async,
                static_cast<OptionalError (ThreadPoolManager::*)()>(&ThreadPoolManager::stop_all),
                this
            );

            if (future.wait_for(timeout) == std::future_status::ready) return future.get();

            this->instant_stop_all();

            throw std::runtime_error("Timeout expired during thread pool shutdown.");
        }


        void instant_stop_all() {
            wait_all();

            auto workers_to_join = worker_registry_.get_all_worker_pointers();

            for (const auto& worker_ptr : workers_to_join) {
                worker_ptr->instant_stop();
            }

            notify_all();

            for (const auto& worker_ptr : workers_to_join) {
                worker_ptr->join();
            }

            wait_all();
            worker_registry_.clear();
        }


        void pause() {
            this->pause_ = true;
        }

        void resume() {
            this->pause_ = false;
            this->notify_all();
        }


        void notify_all() override {
            worker_registry_.notify_all();
        }

        void notify(std::jthread::id thread_id) override {
            worker_registry_.notify(thread_id);
        }


        [[nodiscard]] std::unordered_map<Task::id_t, TaskPointer> get_task_registry() const override {
            return this->task_registry_.get_all_task();
        }

        [[nodiscard]] std::expected<std::map<std::jthread::id, WorkerInfo>, JizhakError> get_workers_info() const override {
            return worker_registry_.get_workers_info();
        }

        [[nodiscard]] std::expected<WorkerInfo, JizhakError> find_worker_info(std::jthread::id thread_id) const override {
            if (const auto worker_info_opt = worker_registry_.get_worker_info(thread_id)) {
                return *worker_info_opt;
            }
            return std::unexpected(JizhakError{JizhakErrorID::worker_not_found});
        }

        [[nodiscard]] std::expected<TaskPointer, JizhakError> find_task(Task::id_t task_id) const override {
            if (auto task_opt = task_registry_.find_task(task_id)) {
                return *task_opt;
            }

            return std::unexpected(JizhakError{JizhakErrorID::task_not_found});
        }


        [[nodiscard]] size_t size() const override {
            return number_workers() + number_tasks();
        }


        [[nodiscard]] size_t number_tasks() const override {
            return this->task_registry_.size();
        }


        [[nodiscard]] size_t number_workers() const override {
            std::scoped_lock lock(mutex_);
            return this->worker_registry_.size();
        }


        [[nodiscard]] bool is_there_task() const override {
            if (this->pending_tasks_)
                return true;
            return false;
        }

        [[nodiscard]] bool is_paused() const override {
            return this->pause_;
        }

        [[nodiscard]] bool is_this_thread() const {
            return this->this_context_.load();
        }

        std::optional<std::deque<TaskPointer>> steal_tasks_from(std::jthread::id victim_id) override {
            return worker_registry_.yield_tasks_from(victim_id);
        }
    };

    template <typename TWorkerRegistry     = WorkerRegistry<>,
              typename TTaskRegistry       = TaskRegistry,
              typename TWorker             = Worker>
    requires (concepts::is_supported_worker<TWorker>)
    struct TPM {
        using DefaultThreadPoolManager = ThreadPoolManager<TWorkerRegistry, TTaskRegistry, TWorker>;

        std::shared_ptr<DefaultThreadPoolManager> tpm_ptr{};

        TPM() = default;

        explicit TPM(unsigned int quantity) : tpm_ptr(std::make_shared<DefaultThreadPoolManager>(quantity)) {}

        DefaultThreadPoolManager* operator->() const {
            return tpm_ptr.operator->();
        }

        std::unordered_map<Task::id_t, TaskPointer> operator[]() const {
            return tpm_ptr->get_task_registry();
        }

        std::expected<WorkerInfo, JizhakError> operator[](std::jthread::id thread_id) const {
            return tpm_ptr->find_worker_info(thread_id);
        }

        std::expected<TaskPointer, JizhakError> operator[](const Task::id_t task_id) const {
            return tpm_ptr->find_task(task_id);
        }

        std::expected<Task::id_t, JizhakError> operator()(TaskPointer& task)  {
            return tpm_ptr->add_task(task);
        }
        template <typename F, typename... Args>
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(const TaskInfo task_info, F&& func, Args&&... args) {
            return tpm_ptr->add_task(TaskInfo(task_info), std::forward<F>(func), std::forward<Args>(args)...);
        }
        template <typename F, typename... Args>
        requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(const TaskInfoField task_info_field, F&& func, Args&&... args) {
            return tpm_ptr->add_task(TaskInfo(task_info_field), std::forward<F>(func), std::forward<Args>(args)...);
        }
        template <typename F, typename... Args>
        requires(!std::is_same_v<std::decay_t<F>, TaskInfo>)
        std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
        operator()(F&& func, Args&&... args) {
            return tpm_ptr->add_task(TaskInfo(), std::forward<F>(func), std::forward<Args>(args)...);
        }
    };

    inline TPM<> make_tpm() {
        auto tpm = TPM<>();
        tpm->register_this_thread();
        return tpm;
    }

    inline TPM<> make_tpm(unsigned int quantity) {
        auto tpm = TPM<>(quantity);
        tpm->register_this_thread();
        return tpm;
    }
} // namespace jzh
