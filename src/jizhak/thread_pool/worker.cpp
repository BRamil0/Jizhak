module jizhak.thread_pool.worker;
import std;

import jizhak.thread_pool.tpm;
import jizhak.thread_pool.tpm_base;
import jizhak.thread_pool.this_thread;
import jizhak.thread_pool.task;

namespace jzh {
    using OptionalError = BaseWorker::OptionalError;

    // BaseWorker: protected
    void BaseWorker::run_loop(std::stop_token token) {
        auto tpm = this_thread::get_tpm().lock();
        if (!tpm) return;

        while (true) {
            Task task_to_run;
            Task::id_t task_id_for_completion = 0;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [this, &token, tpm] {
                    return token.stop_requested() || (!tasks.empty() && !tpm->__is_paused());
                });

                if (token.stop_requested()) return;

                if (is_shutdown.load() && tasks.empty()) return;

                if (tasks.empty() && !is_shutdown.load()) {
                    lock.unlock();

                    if (!steal_task()) {
                        lock.lock();
                        continue;
                    };

                    lock.lock();
                    continue;
                }

                if (!tasks.empty()) {
                    task_to_run = std::move(tasks.front());
                    tasks.pop_front();
                    task_id_for_completion = task_to_run.id;
                }
            }

            if (task_to_run.function) {
                if (task_to_run.function) {
                    try {
                        task_to_run();
                    } catch (const std::exception& e) {
                    } catch (...) {
                    }
                    tpm->task_completed(task_id_for_completion, this->id);
                }
            }
        }
    }

    OptionalError BaseWorker::steal_task() {
        return std::nullopt;
    };

    // BaseWorker: public
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

    void BaseWorker::notify() {
        this->cv.notify_one();
    }

    void BaseWorker::start_shutdown() {
        this->is_shutdown = true;
        cv.notify_one();
    }

    void BaseWorker::join() {
        if (thread.joinable()) {
            thread.join();
        }
    }

    void BaseWorker::instant_stop() {
        thread.request_stop();
    }

    std::optional<std::deque<Task>> BaseWorker::yield_half_of_tasks() {
        std::scoped_lock lock(queue_mutex);

        size_t tasks_to_steal = tasks.size() / 2;
        if (tasks_to_steal == 0 && !tasks.empty())
            tasks_to_steal = 1;

        if (tasks_to_steal == 0)
            return std::nullopt;

        std::deque<Task> stolen_tasks;

        for([[maybe_unused]] size_t _ : std::ranges::iota_view{0uz, tasks_to_steal}) {
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

    // Worker: protected
    OptionalError Worker::steal_task() {
        const auto locked_tpm = this_thread::get_tpm().lock();
        if (!locked_tpm) return std::nullopt;
        ThreadPoolManagerBase* tpm = locked_tpm.get();

        const size_t pool_size = tpm->__number_workers();
        if (pool_size <= 1) return std::nullopt;
        std::uniform_int_distribution<size_t> distribution(0, pool_size - 1);

        for ([[maybe_unused]] size_t _ : std::ranges::iota_view{0uz, pool_size}) {
            const size_t victim_index = distribution(random_generator_);
            if (auto weak_victim = tpm->get_worker_by_index(victim_index); weak_victim.has_value()) {
                if (const auto shared_victim = weak_victim.value().lock()) {
                    if (shared_victim && shared_victim->get_id() != this->get_id()) {

                        // <--- ЗМІНА №5: Прибираємо const тут
                        if (auto stolen_tasks_opt = shared_victim->yield_half_of_tasks()) {
                            if (!stolen_tasks_opt->empty()) {
                                const auto victim_id = shared_victim->get_id();

                                {
                                    std::scoped_lock lock(this->queue_mutex);
                                    // <--- ... і об'єднуємо цикли в один
                                    for (auto& task : *stolen_tasks_opt) {
                                        tpm->notify_steal_task(task.id, this->get_id(), victim_id);
                                        this->tasks.push_back(std::move(task));
                                    }
                                }
                                return std::nullopt;
                            }
                        }
                    }
                }
            }
        }
        return JizhakError{JizhakErrorID::cannot_steal_task};
    }
} // namespace jzh
