module jizhak.thread_pool.worker;
import std;

import jizhak.thread_pool.tpm;
import jizhak.thread_pool.tpm_base;
import jizhak.thread_pool.this_tpm;
import jizhak.thread_pool.task;

namespace jzh {
    using OptionalError = BaseWorker::OptionalError;

    void BaseWorker::run_loop(std::stop_token token) {
        auto locked_tpm = this_thread::get_tpm().lock();
        if (!locked_tpm) return;
        ThreadPoolManagerBase* tpm = locked_tpm.get();

        while (!token.stop_requested()) {
            Task task_to_run;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [this, &token] { return !tasks.empty() || token.stop_requested(); });

                if (token.stop_requested()) return;

                if (tasks.empty()) {
                    lock.unlock();

                    if (!steal_task()) continue;

                    lock.lock();
                    continue;
                }

                task_to_run = std::move(tasks.front());
                tasks.pop_front();
            }

            if (task_to_run.function) {
                task_to_run();
                tpm->task_completed(task_to_run.id, this->id);
            }
        }
    }

    OptionalError BaseWorker::steal_task() {
        return std::nullopt;
    };

    void BaseWorker::start(std::function<void(std::stop_token)> work_function) {
        this->thread = std::jthread(std::move(work_function));
        this->id = thread.get_id();
    }

    void BaseWorker::add_task(Task new_task) {
        {
            std::scoped_lock lock(queue_mutex);
            tasks.push_back(std::move(new_task));
        }
        cv.notify_one();
    }

    std::optional<std::deque<Task>> BaseWorker:: yield_half_of_tasks() {
        std::scoped_lock lock(queue_mutex);

        size_t tasks_to_steal = tasks.size() / 2;
        if (tasks_to_steal == 0 && !tasks.empty()) {
            tasks_to_steal = 1;
        }

        if (tasks_to_steal == 0) {
            return std::nullopt;
        }

        std::deque<Task> stolen_tasks;

        for(size_t _ : std::ranges::iota_view{static_cast<size_t>(0), tasks_to_steal}) {
            stolen_tasks.push_back(std::move(tasks.front()));
            tasks.pop_front();
        }

        return stolen_tasks;
    }

    size_t BaseWorker::size() const {
        std::scoped_lock lock(queue_mutex);
        return tasks.size();
    }

    bool BaseWorker::empty() const {
        std::scoped_lock lock(queue_mutex);
        return tasks.empty();
    }

    std::thread::id BaseWorker::get_id() const {
        return this->id;
    }

    OptionalError Worker::steal_task() {
        auto locked_tpm = this_thread::get_tpm().lock();
        if (!locked_tpm) return std::nullopt;
        ThreadPoolManagerBase* tpm = locked_tpm.get();

        size_t pool_size = tpm->__number_workers();
        if (pool_size <= 1) return std::nullopt;
        std::uniform_int_distribution<size_t> distribution(0, pool_size - 1);

        for (size_t _ : std::ranges::iota_view{static_cast<size_t>(0), pool_size}) {
            size_t victim_index = distribution(random_generator_);

            BaseWorker* base_victim = tpm->get_worker_by_index(victim_index);
            if (base_victim && base_victim->get_id() != this->get_id()) {
                if (auto stolen_tasks_opt = base_victim->yield_half_of_tasks()) {
                    if (!stolen_tasks_opt->empty()) {
                        const auto victim_id = base_victim->get_id();

                        for (const auto& task : *stolen_tasks_opt)
                            tpm->notify_steal_task(task.id, this->get_id(), victim_id);

                        {
                            std::scoped_lock lock(this->queue_mutex);
                            for (auto& task : *stolen_tasks_opt)
                                this->tasks.push_back(std::move(task));
                        }
                        return std::nullopt;
                    }
                }
            }
        }

        return JizhakError{JizhakErrorID::cannot_steal_task};
    }
} // namespace jzh
