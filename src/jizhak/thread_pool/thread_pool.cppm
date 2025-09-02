/// Загальний модуль для реімпорту всіх модулів thread_pool.
export module jizhak.thread_pool;

export import jizhak.thread_pool.tpm;
export import jizhak.thread_pool.utilities_tpm;
export import jizhak.thread_pool.tpm_base;
export import jizhak.thread_pool.worker;
export import jizhak.thread_pool.task;
export import jizhak.thread_pool.this_thread;
export import jizhak.thread_pool.async;

/// Псевдонім для зручного використання using.
export namespace jzh::thread::using_this_thread::this_thread {
    using namespace jzh::thread::this_thread;
}

/// Псевдонім для зручного використання using.
export namespace jzh::using_thread::thread {
    using namespace jzh::thread;
}