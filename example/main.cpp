import std;
import jizhak;

using namespace std::string_literals;

int main() {
    if constexpr (jzh::compiler::msvc)
        jzh::println("Compiler: MSVC");
    else if constexpr (jzh::compiler::clang)
        jzh::println("Compiler: Clang");
    else if constexpr (jzh::compiler::gcc)
        jzh::println("Compiler: GCC");
    else
        jzh::println("Compiler: Unknown");
    return 0;
}