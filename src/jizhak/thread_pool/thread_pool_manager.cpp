module jizhak.thread_pool.tpm;
import std;

import jizhak.thread_pool.task;
import jizhak.thread_pool.worker;
import jizhak.thread_pool.this_tpm;

namespace jzh {
    // ThreadPoolManager: protected
    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::jthread::id ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::create_worker() {
        auto worker_ptr = std::make_shared<TWorker>();

        auto work_function = [this, worker_raw = worker_ptr.get()](std::stop_token token) {
            this_thread::set_tpm(this->weak_from_this());
            worker_raw->run_loop(token);
        };
        worker_ptr->start(std::move(work_function));

        {
            std::scoped_lock lock(info_table_mutex_);
            this->info_table_[worker_ptr->get_id()] = TInfoTable(worker_ptr);
        }

        return worker_ptr->get_id();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<std::vector<std::jthread::id>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::create_worker(unsigned int quantity) {
        if (quantity == 0)
            return std::unexpected<JizhakError>(JizhakErrorID::zero_transferred);

        std::vector<std::jthread::id> ids{};
        ids.reserve(quantity);

        for ([[maybe_unused]] auto _: std::ranges::iota_view{0u ,quantity})
            ids.push_back(this->create_worker());
        return ids;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::stop_worker(std::jthread::id thread_id) {
        std::shared_ptr<TWorker> worker_to_stop;

        {
            std::scoped_lock lock(info_table_mutex_);

            auto it = info_table_.find(thread_id);

            if (it == info_table_.end())
                return JizhakError{JizhakErrorID::worker_not_found};

            worker_to_stop = std::move(*it);
            info_table_.erase(it);
        }

        worker_to_stop->start_shutdown();
        worker_to_stop->join();

        return std::nullopt;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<std::weak_ptr<TWorker>, JizhakError> ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::get_worker(std::jthread::id thread_id) {
        if (auto& it = info_table_.find(thread_id); it != info_table_.end())
            return it;

        return std::unexpected<JizhakError>(JizhakErrorID::worker_not_found);
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<std::weak_ptr<TWorker>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::get_worker_by_index(size_t index) override {
        std::scoped_lock lock(info_table_mutex_);
        if (index >= info_table_.size())
            return std::unexpected<JizhakError>(JizhakErrorID::index_overrun);

        auto it = info_table_.begin();
        std::advance(it, index);
        return it->second.get_worker();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<Task::id_t, JizhakError> ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::create_task(Task& task, TaskInfo& task_info) {
        if (info_table_.empty())
            return std::unexpected(JizhakError(JizhakErrorID::no_workers_available));

        if (task.id != task_info.id)
            return std::unexpected(JizhakError(JizhakErrorID::identifiers_are_different));

        auto first_entry_iterator = info_table_.begin();

        auto node_handle = info_table_.extract(first_entry_iterator);
        TInfoTable& info_table_obj = node_handle.mapped();

        ++node_handle.key().pending_tasks;

        info_table_.insert(std::move(node_handle));

        info_table_obj.add_task(task_info);

        ++pending_tasks_;

        info_table_obj->add_task(std::move(task));

        return task.id;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::delete_task(Task::id_t task_id) {}

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::delete_task(Task::id_t task_id, std::jthread::id thread_id) {}


    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::task_completed(Task::id_t task_id, std::jthread::id thread_id) override {}

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::notify_steal_task(Task::id_t task_id,
        std::jthread::id to_thread_id, std::jthread::id from_thread_id) override {}

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::__number_workers() const override {
        return this->number_workers();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    bool ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::__is_paused() const override {
        return this->is_paused();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<Task::id_t, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::__add_task(Task& task, TaskInfo& task_info) {
        std::scoped_lock lock(info_table_mutex_);
        return this->create_task(task, task_info);
    }

    // ThreadPoolManager: public
    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::ThreadPoolManager(unsigned int quantity) {
        std::scoped_lock lock(info_table_mutex_);
        auto result = this->create_worker(quantity);
        if (!result)
            throw std::runtime_error("Failed to create initial workers: " + result.error().message());
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::~ThreadPoolManager() override {
        this->stop_all();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<const typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::InfoTable&, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::operator[]() const {
        return this->get_info_table();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<const TInfoTable&, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::operator[](std::jthread::id thread_id) const {
        return this->search_worker_for(thread_id);
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<const TaskInfo, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::operator[](const Task::id_t task_id) const {
        return this->search_task_for(task_id);
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<Task::id_t, JizhakError> ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::operator()(Task& task, TaskInfo& task_info) {
        return this->create_task(task, task_info);
    }

    template <typename TInfoTable, typename TInfoSorter, typename TWorker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <typename F, typename ... Args> std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::operator()(std::tuple<Task, TaskInfo> task_bundle) {
        return this->add_task(std::forward<std::tuple<Task, TaskInfo>>(task_bundle));
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <typename F, typename... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::operator()(F&& func, Args&&... args) {
        return this->add_task(std::forward<F>(func), std::forward<Args>(args)...);
    }

    template <typename TInfoTable, typename TInfoSorter, typename TWorker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <typename F, typename ... Args> std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::operator()(TaskInfo task_info, F&& func, Args&&... args) {
        return this->add_task(std::forward<TaskInfo>(task_info), std::forward<F>(func), std::forward<Args>(args)...);
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<std::jthread::id, JizhakError> ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::add_worker() {
        std::scoped_lock lock(info_table_mutex_);
        return this->create_worker();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::remove_worker(std::jthread::id thread_id) {
        return this->stop_worker(thread_id);
    }


    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<Task::id_t, JizhakError> ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::add_task(Task& task, TaskInfo& task_info) {
        std::scoped_lock lock(info_table_mutex_);
        return this->create_task(task, task_info);
    }

    template <typename TInfoTable, typename TInfoSorter, typename TWorker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <typename F, typename ... Args> std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>,
    Task::id_t>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::add_task(std::tuple<Task, TaskInfo> task_bundle) {
        return std::apply(
           [this](Task& task, TaskInfo& task_info) {
               std::scoped_lock lock(info_table_mutex_);
               return this->create_task(task, task_info);
           },
           task_bundle
       );
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <typename F, typename... Args>
    std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::add_task(F&& func, Args&&... args) {
        return this->add_task(task_info_set_id(TaskInfo()), std::forward<F>(func), std::forward<Args>(args)...);
    }

    template <typename TInfoTable, typename TInfoSorter, typename TWorker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <typename F, typename ... Args> std::expected<std::tuple<std::future<std::invoke_result_t<F, Args...>>, Task::id_t>, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::add_task(TaskInfo task_info, F&& func, Args&&... args) {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto bound_func = std::bind_front(std::forward<F>(func), std::forward<Args>(args)...);
        std::packaged_task<ReturnType()> packaged_task(std::move(bound_func));

        std::future<ReturnType> future_result = packaged_task.get_future();
        Task::Function work_function = [pt = std::move(packaged_task)]() mutable {
            pt();
        };

        auto [task, new_task_info] = make_task(std::move(work_function), task_info);

        std::scoped_lock lock(info_table_mutex_);

        if (auto creation_result = create_task(task, new_task_info); !creation_result)
            return std::unexpected(creation_result.error());

        return std::make_tuple(std::move(future_result), new_task_info.id);
    }


    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::remove_task(Task::id_t task_id) {}

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::remove_task(Task::id_t task_id, std::jthread::id thread_id) {}

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    void ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::wait_all() {
        std::unique_lock lock(wait_mutex_);

        wait_cv_.wait(lock, [this] {
            return pending_tasks_.load() == 0;
        });
    }


    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <class Rep, class Period>
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::wait_all(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock lock(wait_mutex_);

        bool success = wait_cv_.wait_for(lock, timeout, [this] {
            return pending_tasks_.load() == 0;
        });

        if (success) return std::nullopt;
        return JizhakError{JizhakErrorID::timeout_expired};
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::stop_all() {
        wait_all();

        std::vector<TInfoSorter> keys_to_stop;
        {
            std::scoped_lock lock(info_table_mutex_);
            for (const auto& pair : info_table_) {
                keys_to_stop.push_back(pair.first);
            }
        }

        for (const auto& key : keys_to_stop) {
            stop_worker(key.id);
        }

        return std::nullopt;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    template <class Rep, class Period>
    typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::OptionalError
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::stop_all(const std::chrono::duration<Rep, Period>& timeout) {
        auto future = std::async(std::launch::async, &ThreadPoolManager::stop_all, this);

        if (future.wait_for(timeout) == std::future_status::ready)
            return future.get();

        throw std::runtime_error("Timeout expired during thread pool shutdown.");
    }


    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    void ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::pause() {
        this->pause_ = true;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    void ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::resume() {
        this->pause_ = false;
        this->notify_all();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    void ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::notify_all() {
        std::scoped_lock lock(info_table_mutex_);
        for (const auto& info_ptr : info_table_) {
            info_ptr->notify();
        }
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    void ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::notify(std::jthread::id thread_id) {
        std::scoped_lock lock(info_table_mutex_);
        if (auto it = info_table_.find(thread_id); it != info_table_.end())
            if (auto locked_info = it->second.lock())
                locked_info->notify();
    }


    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<const typename ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::InfoTable&, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::get_info_table() const {
        return this->info_table_;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<const TInfoTable&, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::search_worker_for(std::jthread::id thread_id) const {
        std::scoped_lock lock(info_table_mutex_);

        for (const auto& pair : info_table_)
            if (pair.first.id == thread_id)
                return pair.second;

        return std::unexpected(JizhakError{JizhakErrorID::worker_not_found});
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    std::expected<const TaskInfo, JizhakError>
    ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::search_task_for(Task::id_t task_id) const {
        std::scoped_lock lock(info_table_mutex_);

        for (auto&& info : info_table_ | std::views::values)
            if (auto& it = std::find(info, task_id); it != info.end())
                return it;

        return std::unexpected(JizhakError{JizhakErrorID::task_not_found});
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::size() const {
        return this->number_workers();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::number_tasks() const {
        return this->pending_tasks_;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    size_t ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::number_workers() const {
        std::scoped_lock lock(info_table_mutex_);
        return this->workers_.size();
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    [[nodiscard]] bool ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::is_there_task() const {
        if (this->pending_tasks_)
            return true;
        return false;
    }

    template <typename TInfoTable = InformationTable, typename TInfoSorter = InformationSorter, typename TWorker = Worker>
    requires (concepts::is_supported_info_table<TInfoTable, TWorker> && concepts::is_supported_worker<TWorker>)
    [[nodiscard]] bool ThreadPoolManager<TInfoTable, TInfoSorter, TWorker>::is_paused() const {
        return this->pause_;
    }

} // namespace jzh
