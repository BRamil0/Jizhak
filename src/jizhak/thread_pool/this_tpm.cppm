export module jizhak.thread_pool.this_tpm;

import std;

namespace jzh {
    class ThreadPoolManager;
}

namespace jzh::this_thread {
    inline thread_local std::weak_ptr<ThreadPoolManager> current_tpm{};
}

export namespace jzh::this_thread {
    inline void set_tpm(const std::weak_ptr<ThreadPoolManager>& tpm) {
        current_tpm = tpm;
    }

    [[nodiscard]] inline std::weak_ptr<ThreadPoolManager> get_tpm() {
        return current_tpm;
    }
}