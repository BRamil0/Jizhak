module jizhak.thread_pool.tpm;
import std;

import jizhak.thread_pool.task;
import jizhak.thread_pool.worker;
import jizhak.thread_pool.this_tpm;

namespace jzh {
    // ThreadPoolManager: protected
    template <template <typename...> class TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::jthread::id ThreadPoolManager<TContainer, TWorker>::create_worker() {
        auto worker_ptr = std::make_shared<TWorker>();

        auto work_function = [this, worker_raw = worker_ptr.get()](std::stop_token token) {
            this_thread::set_tpm(this->weak_from_this());
            worker_raw->run_loop(token);
        };

        worker_ptr->start(std::move(work_function));

        workers_.push_back(worker_ptr);
        {
            std::scoped_lock lock(tables_mutex_);
            worker_for_id_[worker_ptr->get_id()] = worker_ptr;
            table_worker_stats_[worker_ptr->get_id()] = SynchronizedWorkerStats{};
            table_task_infos_[worker_ptr->get_id()] = SynchronizedTaskInfos{};
        }

        return worker_ptr->get_id();
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<std::vector<std::jthread::id>, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::create_worker(unsigned int quantity) {
        if (quantity == 0)
            return std::unexpected<JizhakError>(JizhakErrorID::zero_transferred);

        std::vector<std::jthread::id> ids{};
        ids.reserve(quantity);

        for ([[maybe_unused]] auto _: std::ranges::iota_view{0u ,quantity})
            ids.push_back(this->create_worker());
        return ids;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::stop_worker(std::jthread::id thread_id) {
        std::shared_ptr<TWorker> worker_to_stop;

        {
            std::scoped_lock lock(workers_mutex_);

            auto it = std::find_if(workers_.begin(), workers_.end(),
                [&](const std::shared_ptr<TWorker>& ptr) {
                    return ptr->get_id() == thread_id;
                });

            if (it == workers_.end())
                return JizhakError{JizhakErrorID::worker_not_found};

            worker_to_stop = std::move(*it);
            workers_.erase(it);

            worker_for_id_.erase(thread_id);
        }

        worker_to_stop->start_shutdown();

        {
            std::scoped_lock lock(tables_mutex_);
            table_worker_stats_.erase(thread_id);
            table_task_infos_.erase(thread_id);
            worker_for_id_.erase(thread_id);
        }

        worker_to_stop->join();
        return std::nullopt;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const TWorker*, JizhakError> ThreadPoolManager<TContainer, TWorker>::get_worker(std::jthread::id thread_id) {
        if (auto& it = worker_for_id_[thread_id]; !it == worker_for_id_.end())
            return it;

        return std::unexpected<JizhakError>(JizhakErrorID::worker_not_found);
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<Task::id_t, JizhakError> ThreadPoolManager<TContainer, TWorker>::create_task(Task& task) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::delete_task(Task::id_t task_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::delete_task(Task::id_t task_id, std::jthread::id thread_id) {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::task_completed(Task::id_t task_id, std::jthread::id thread_id) override {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::notify_steal_task(Task::id_t task_id,
        std::jthread::id to_thread_id, std::jthread::id from_thread_id) override {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::weak_ptr<TWorker> ThreadPoolManager<TContainer, TWorker>::get_worker_by_index(size_t index) override {
        std::scoped_lock lock(workers_mutex_);
        return this->workers_.at(index);
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::__number_workers() const override {
        return this->number_workers();
    }

    template <template <typename...> class TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    bool ThreadPoolManager<TContainer, TWorker>::__is_paused() const override {
        return this->is_paused();
    }

    // ThreadPoolManager: public
    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::ThreadPoolManager(unsigned int quantity) {
        std::scoped_lock lock(workers_mutex_);
        auto result = this->create_worker(quantity);
        if (!result)
            throw std::runtime_error("Failed to create initial workers: " + result.error().message());
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    ThreadPoolManager<TContainer, TWorker>::~ThreadPoolManager() override {
        this->stop_all();
    }


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const WorkerStats&, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::operator[](std::jthread::id thread_id) const {
        return this->search_worker_for(thread_id);
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const TaskInfo, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::operator[](const Task::id_t task_id) const {
        return this->search_task_for(task_id);
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    template <typename F, typename... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::operator()(F&& func, Args&&... args) {}

    template <template <typename...> class TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<std::jthread::id, JizhakError> ThreadPoolManager<TContainer, TWorker>::add_worker() {
        std::scoped_lock lock(workers_mutex_);
        return this->create_worker();
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::remove_worker(std::jthread::id thread_id) {
        return this->stop_worker(thread_id);
    }


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    template <typename F, typename... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::add_task(F&& func, Args&&... args) {}


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::remove_task(Task::id_t task_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::remove_task(Task::id_t task_id, std::jthread::id thread_id) {}

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    void ThreadPoolManager<TContainer, TWorker>::wait_all() {
        std::unique_lock lock(wait_mutex_);

        wait_cv_.wait(lock, [this] {
            return pending_tasks_.load() == 0;
        });
    }


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    template <class Rep, class Period>
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::wait_all(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock lock(wait_mutex_);

        bool success = wait_cv_.wait_for(lock, timeout, [this] {
            return pending_tasks_.load() == 0;
        });

        if (success) return std::nullopt;
        return JizhakError{JizhakErrorID::timeout_expired};
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError ThreadPoolManager<TContainer, TWorker>::stop_all() {
        wait_all();

        std::vector<std::jthread::id> ids_to_stop;

        {
            std::scoped_lock lock(workers_mutex_);
            ids_to_stop.reserve(worker_for_id_.size());
            for (const auto& pair : worker_for_id_) {
                ids_to_stop.push_back(pair.first);
            }
        }

        for (const auto& id : ids_to_stop) {
            stop_worker(id);
        }

        return std::nullopt;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    template <class Rep, class Period>
    typename ThreadPoolManager<TContainer, TWorker>::OptionalError
    ThreadPoolManager<TContainer, TWorker>::stop_all(const std::chrono::duration<Rep, Period>& timeout) {
        auto future = std::async(std::launch::async, &ThreadPoolManager::stop_all, this);

        if (future.wait_for(timeout) == std::future_status::ready)
            return future.get();

        throw std::runtime_error("Timeout expired during thread pool shutdown.");
    }


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    void ThreadPoolManager<TContainer, TWorker>::pause() {
        this->pause_ = true;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    void ThreadPoolManager<TContainer, TWorker>::resume() {
        this->pause_ = false;
        this->notify_all();
    }

    template <template <typename...> class TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    void ThreadPoolManager<TContainer, TWorker>::notify_all() {
        std::scoped_lock lock(workers_mutex_);
        for (const auto& worker_ptr : workers_) {
            worker_ptr->notify();
        }
    }

    template <template <typename...> class TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    void ThreadPoolManager<TContainer, TWorker>::notify(std::jthread::id thread_id) {
        std::scoped_lock lock(workers_mutex_);
        if (auto it = worker_for_id_.find(thread_id); it != worker_for_id_.end())
            if (auto locked_worker = it->second.lock())
                locked_worker->notify();
    }


    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const typename ThreadPoolManager<TContainer, TWorker>::TableWorker, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::get_table_worker() const {
        return this->table_worker_stats_;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const typename ThreadPoolManager<TContainer, TWorker>::TableTask, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::get_table_task() const {
        return this->table_task_infos_;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const WorkerStats&, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::search_worker_for(std::jthread::id thread_id) const {
        std::scoped_lock lock(tables_mutex_);

        if (auto it = table_worker_stats_.find(thread_id); it != table_worker_stats_.end())
            return it->second.stats;
        return std::unexpected(JizhakError{JizhakErrorID::worker_not_found});
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    std::expected<const TaskInfo, JizhakError>
    ThreadPoolManager<TContainer, TWorker>::search_task_for(Task::id_t task_id) const {
        std::scoped_lock lock(tables_mutex_);

        for (auto&& value : table_task_infos_ | std::views::values)
            for (const auto& task_info : value.infos)
                if (task_info.id == task_id)
                    return task_info;

        return std::unexpected(JizhakError{JizhakErrorID::task_not_found});
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::size() const {
        return this->number_workers();
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::number_tasks() const {
        return this->pending_tasks_;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TContainer, TWorker>::number_workers() const {
        std::scoped_lock lock(workers_mutex_);
        return this->workers_.size();
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    [[nodiscard]] bool ThreadPoolManager<TContainer, TWorker>::is_there_task() const {
        if (this->pending_tasks_)
            return true;
        return false;
    }

    template <template <typename...> typename TContainer, typename TWorker>
    requires (is_supported_container<TContainer, std::shared_ptr<TWorker>> && is_supported_worker<TWorker>)
    [[nodiscard]] bool ThreadPoolManager<TContainer, TWorker>::is_paused() const {
        return this->pause_;
    }

} // namespace jzh
