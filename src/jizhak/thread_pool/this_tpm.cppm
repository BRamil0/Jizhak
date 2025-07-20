export module jizhak.thread_pool.this_tpm;

import jizhak.thread_pool.tpm_base;
import std;


namespace jzh::this_thread {
    inline thread_local std::weak_ptr<ThreadPoolManagerBase> current_tpm{};
}

export namespace jzh::this_thread {
    inline void set_tpm(const std::weak_ptr<ThreadPoolManagerBase>& tpm) {
        current_tpm = tpm;
    }

    [[nodiscard]] inline std::weak_ptr<ThreadPoolManagerBase> get_tpm() {
        return current_tpm;
    }
}