module jizhak.thread_pool.worker;
import std;

import jizhak.thread_pool.tpm;
import jizhak.thread_pool.task;

namespace jzh {
    void Worker::run_loop(std::stop_token token) {
        while (!token.stop_requested()) {
            Task task_to_run;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [this, &token] { return !tasks.empty() || token.stop_requested(); });

                if (token.stop_requested()) return;

                task_to_run = std::move(tasks.front());
                tasks.pop_front();
            }

            if (task_to_run.function) {
                task_to_run();
            }
        }
    }

    void Worker::start(std::function<void(std::stop_token)> work_function) {
        this->thread = std::jthread(std::move(work_function));
        this->id = thread.get_id();
    }

    void Worker::add_task(Task new_task) {
        {
            std::scoped_lock lock(queue_mutex);
            tasks.push_back(std::move(new_task));
        }
        cv.notify_one();
    }

    size_t Worker::size() const {
        std::scoped_lock lock(queue_mutex);
        return tasks.size();
    }

    bool Worker::empty() const {
        std::scoped_lock lock(queue_mutex);
        return tasks.empty();
    }
} // namespace jzh
