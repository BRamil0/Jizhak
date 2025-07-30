export module jizhak.thread_pool.async;

import std;
export import jizhak.error;
export import jizhak.thread_pool.this_thread;
export import jizhak.thread_pool.tpm_base;
export import jizhak.thread_pool.task;

export namespace std {
    template <typename T, typename... Args>
    struct coroutine_traits<std::future<T>, Args...> {
        struct promise_type {
            std::promise<T> p;
            auto get_return_object() { return p.get_future(); }
            std::suspend_never initial_suspend() noexcept { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_value(T value) { p.set_value(std::move(value)); }
            void unhandled_exception() { p.set_exception(std::current_exception()); }
        };
    };

    template <typename... Args>
    struct coroutine_traits<std::future<void>, Args...> {
        struct promise_type {
            std::promise<void> p;
            auto get_return_object() { return p.get_future(); }
            std::suspend_never initial_suspend() noexcept { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() { p.set_value(); }
            void unhandled_exception() { p.set_exception(std::current_exception()); }
        };
    };
} // namespace std

export namespace jzh {
    // FutureAwaiter залишається без змін, він все ще правильний
    template <typename T>
    struct FutureAwaiter {
        std::future<T> future;
        std::weak_ptr<ThreadPoolManagerBase> tpm_ptr;

        bool await_ready() noexcept {
            return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            auto tpm = tpm_ptr.lock();
            if (!tpm) {
                std::jthread([this, handle] {
                    future.wait();
                    handle.resume();
                }).detach();
                return;
            }
            auto continuation_task = make_task([this, handle] {
                future.wait();
                handle.resume();
            });
            (void)tpm->add_task(continuation_task);
        }

        T await_resume() {
            return future.get();
        }
    };

    template <>
    struct FutureAwaiter<void> {
        std::future<void> future;
        std::weak_ptr<ThreadPoolManagerBase> tpm_ptr;

        bool await_ready() noexcept {
            return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            auto tpm = tpm_ptr.lock();
            if (!tpm) {
                std::jthread([this, handle] {
                    future.wait();
                    handle.resume();
                }).detach();
                return;
            }
            auto continuation_task = make_task([this, handle] {
                future.wait();
                handle.resume();
            });
            (void)tpm->add_task(continuation_task);
        }
        void await_resume() {
            future.get();
        }
    };

    template <typename T>
    auto operator co_await(std::expected<std::pair<std::future<T>, Task::id_t>, JizhakError>&& expected_result) {
        if (!expected_result)
            throw expected_result.error();

        auto tpm_ptr = this_thread::get_tpm();
        auto& [future, task_id] = *expected_result;
        return FutureAwaiter<T>{ std::move(future), tpm_ptr };
    }


    template <typename T>
    auto operator co_await(std::pair<std::future<T>, TaskPointer>&& result_pair) {
        auto tpm_ptr = this_thread::get_tpm();
        return FutureAwaiter<T>{ std::move(result_pair.first), tpm_ptr };
    }
} // namespace jzh