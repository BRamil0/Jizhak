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
            TaskPointer task_to_run;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [this, &token, tpm] {
                    return token.stop_requested() || (!tasks.empty() && !tpm->is_paused());
                });

                if (token.stop_requested()) return;

                if (is_shutdown.load() && tasks.empty()) return;

                if (tasks.empty() && !is_shutdown.load()) {
                    lock.unlock();

                    if (!steal_task()) {
                        lock.lock();
                        continue;
                    }

                    lock.lock();
                    continue;
                }

                if (!tasks.empty()) {
                    task_to_run = std::move(tasks.front());
                    tasks.pop_front();
                }
            }

            if (task_to_run->function) {
                try {
                    task_to_run->task_info.status = TaskStatus::in_progress;
                    if (auto result = task_to_run(); result.has_value()) {
                        task_to_run->task_info.status = TaskStatus::completed_with_error;
                        task_to_run->task_info.error = std::make_exception_ptr<JizhakError>(result.value());
                    } else
                        task_to_run->task_info.status = TaskStatus::completed;

                } catch (const std::exception&) {
                    task_to_run->task_info.status = TaskStatus::error;
                    task_to_run->task_info.error = std::current_exception();

                } catch (...) {
                    task_to_run->task_info.status = TaskStatus::error_no_exception;
                    task_to_run->task_info.error = std::current_exception();

                }

                tpm->_task_completed(task_to_run->task_info.id, this->id);
            }

        }
    }

    OptionalError BaseWorker::steal_task() {
        return std::nullopt;
    }

    // BaseWorker: public
    void BaseWorker::start(std::function<void(std::stop_token)> work_function) {
        this->thread = std::jthread(std::move(work_function));
        this->id = thread.get_id();
    }

    void BaseWorker::add_task(TaskPointer new_task) {
        {
            std::scoped_lock lock(queue_mutex);
            tasks.push_back(std::move(new_task));
        }
        cv.notify_one();
    }

    std::optional<JizhakError> BaseWorker::remove_task([[maybe_unused]] Task::id_t task_id) {
        std::scoped_lock lock(queue_mutex);
        return std::nullopt;
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

    std::optional<std::deque<TaskPointer>> BaseWorker::yield_half_of_tasks() {
        std::scoped_lock lock(queue_mutex);

        size_t tasks_to_steal = tasks.size() / 2;
        if (tasks_to_steal == 0 && !tasks.empty())
            tasks_to_steal = 1;

        if (tasks_to_steal == 0)
            return std::nullopt;

        std::deque<TaskPointer> stolen_tasks;

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
        const auto tpm_weak = this_thread::get_tpm();
        auto tpm = tpm_weak.lock();
        if (!tpm) return std::nullopt;

        auto workers_info_map_exp = tpm->get_workers_info();
        if (!workers_info_map_exp) return std::nullopt;

        auto workers_info_map = *workers_info_map_exp;

        std::vector<std::jthread::id> victim_ids;
        for (const auto& [thread_id, info] : workers_info_map) {
            if (thread_id != this->get_id() && !info.is_shutting_down) {
                victim_ids.push_back(thread_id);
            }
        }

        if (victim_ids.empty()) {
            return JizhakError{JizhakErrorID::cannot_steal_task};
        }

        std::ranges::shuffle(victim_ids, random_generator_);

        for (const auto& victim_id : victim_ids) {
            if (auto stolen_tasks_opt = tpm->steal_tasks_from(victim_id)) {
                if (stolen_tasks_opt && !stolen_tasks_opt->empty()) {

                    for (const auto& task : *stolen_tasks_opt)
                        tpm->_on_task_stolen(task->task_info.id, this->get_id(), victim_id);

                    {
                        std::scoped_lock lock(this->queue_mutex);
                        for (auto& task : *stolen_tasks_opt)
                            this->tasks.push_back(std::move(task));
                    }
                    return std::nullopt;
                }
            }
        }

        return JizhakError{JizhakErrorID::cannot_steal_task};
    }
} // namespace jzh
