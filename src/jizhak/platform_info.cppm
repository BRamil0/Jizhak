export module jizhak.platform_info;

export namespace jzh::compiler {
    //==========================================================================
    // Компілятор (Compiler)
    //==========================================================================

    #if defined(__clang__) || defined(__APPLE_CC__)
    inline constexpr bool clang = true;
    #else
    inline constexpr bool clang = false;
    #endif

    #if defined(_MSC_VER) && !defined(__clang__)
        inline constexpr bool msvc = true;
    #else
        inline constexpr bool msvc = false;
    #endif

    #if defined(__GNUC__) && !defined(__clang__)
    inline constexpr bool gcc = true;
    #else
    inline constexpr bool gcc = false;
    #endif
}

export namespace jzh::os {
    //==========================================================================
    // Операційна система (Operating System)
    //==========================================================================

    #if defined(_WIN32)
    inline constexpr bool windows = true;
    #else
    inline constexpr bool windows = false;
    #endif

    #if defined(__linux__)
    inline constexpr bool linux = true;
    #else
    inline constexpr bool linux = false;
    #endif

    #if defined(__APPLE__) && defined(__MACH__)
    inline constexpr bool macos = true;
    #else
    inline constexpr bool macos = false;
    #endif

    #if defined(__FreeBSD__)
    inline constexpr bool freebsd = true;
    #else
    inline constexpr bool freebsd = false;
    #endif

    #if defined(__android__)
    inline constexpr bool android = true;
    #else
    inline constexpr bool android = false;
    #endif

    #if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
        inline constexpr bool unix = true;
    #else
        inline constexpr bool unix = false;
    #endif
}

export namespace jzh::architecture {
    //==========================================================================
    // Архітектура (Architecture)
    //==========================================================================

    #if defined(_M_X64) || defined(__x86_64__)
        inline constexpr bool x64 = true;
    #else
        inline constexpr bool x64 = false;
    #endif

    #if defined(_M_IX86) || defined(__i386__)
        inline constexpr bool x86 = true;
    #else
        inline constexpr bool x86 = false;
    #endif

    #if defined(__aarch64__) || defined(_M_ARM64)
        inline constexpr bool arm64 = true;
    #else
        inline constexpr bool arm64 = false;
    #endif

    #if defined(__arm__) || defined(_M_ARM)
        inline constexpr bool arm = true;
    #else
        inline constexpr bool arm = false;
    #endif

    #if defined(__riscv)
        inline constexpr bool riscv = true;
        #if __riscv_xlen == 64
            inline constexpr bool riscv64 = true;
            inline constexpr bool riscv32 = false;
        #elif __riscv_xlen == 32
            inline constexpr bool riscv64 = false;
            inline constexpr bool riscv32 = true;
        #else
            inline constexpr bool riscv64 = false;
            inline constexpr bool riscv32 = false;
        #endif
    #else
        inline constexpr bool riscv = false;
        inline constexpr bool riscv64 = false;
        inline constexpr bool riscv32 = false;
    #endif

}

export namespace jzh::standard {
    //==========================================================================
    // Стандарт C++ (C++ Standard)
    //==========================================================================

    #if defined(_MSC_VER) && !defined(__clang__)
        #define JZH_CPP_VERSION _MSVC_LANG
    #else
        #define JZH_CPP_VERSION __cplusplus
    #endif

    #if JZH_CPP_VERSION >= 202602L
        inline constexpr short cplusplus = 26;
    #elif JZH_CPP_VERSION >= 202302L
        inline constexpr short cplusplus = 23;
    #elif JZH_CPP_VERSION >= 202002L
        inline constexpr short cplusplus = 20;
    #elif JZH_CPP_VERSION >= 201703L
        inline constexpr short cplusplus = 17;
    #elif JZH_CPP_VERSION >= 201402L
        inline constexpr short cplusplus = 14;
    #elif JZH_CPP_VERSION >= 201103L
        inline constexpr short cplusplus = 11;
    #elif JZH_CPP_VERSION >= 199711L
        inline constexpr short cplusplus = 98;
    #else
        inline constexpr short cplusplus = 0;
    #endif

    #undef JZH_CPP_VERSION

}