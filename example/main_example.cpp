#if defined(USE_OF_STD_MODULE)
import std;
#else
#include <future>
#include <coroutine>
#endif

import jizhak;


std::future<int> add_2(int x = 2, jzh::reduction::u8str text = u8"Додаємо 2 до {}:") {
    const int result = co_await jzh::this_thread::add_task([x, text] {
        jzh::println(text, x);
        return 2;
    });

    co_return x + result;
}

int main() {
    jzh::println("Jižak: Example");

    try {
        auto tpm = jzh::make_tpm(4);

        auto result1 = tpm->add_task([] {
            jzh::println("Привіт світ!");
            return add_2(50, u8"Додаємо {} до 2: ");
        });

        auto result2 = tpm->add_task([]{ return add_2(4); });
        auto [value, error] = jzh::this_thread::add_task([]{ return add_2(); });

        jzh::println("Результат 50 + 2 = {}", result1.value().first.get().get());
        jzh::println("Результат 4 + 2 = {}", result2.value().first.get().get());
        jzh::println("Результат 2 + 2 = {}", value.get().get());

        tpm->wait_all(std::chrono::minutes(1));

        jzh::this_thread::add_task([] {
            jzh::this_thread::add_task(
                jzh::make_task([] {
                    jzh::println("clang: {}", jzh::compiler::clang);
                },
                jzh::TaskInfoField{.priority = 50}
                ));

            jzh::this_thread::add_task(
                jzh::TaskPointer(
                    std::make_shared<jzh::Task>(
                        jzh::TaskInfo(jzh::TaskInfoField{.priority = 150}),
                        [] {
                            jzh::println("msvc: {}", jzh::compiler::msvc);
                        })));

            jzh::this_thread::add_task([] {
                jzh::TaskInfoField{.priority = 150},
                jzh::println("gcc: {}", jzh::compiler::gcc);
            });
        });

        tpm->stop_all(std::chrono::minutes(1));

    } catch (const std::exception& e) {
        jzh::ceio.println("A critical error occurred: {}", e.what());
        return 1;
    }
    return 0;
}