import std;
import jizhak;

using namespace std::string_literals;

std::future<int> test_func() {
    jzh::println("HELLO");
    const int result = co_await jzh::this_thread::add_task([] {
        jzh::println("Як справи");
        return 1;
    });

    co_return result + 100;
}


int main() {
    using namespace jzh;;
    try {
        auto tpm = jzh::make_tpm(4);
        auto еу = tpm->add_task([] {
            jzh::println("Привіт світ!");
        });

        this_thread::add_task([] {
            for (int i = 0; i <= 100; i++) {
                println("Число: {}", i);
            };
            jzh::println("Все добре");
        });

        auto f = tpm->add_task(test_func);

        jzh::println("{}", f.value().first.get().get());
        tpm->stop_all(std::chrono::minutes(1));
    } catch (const std::exception e) {
        jzh::ceio.println("A critical error occurred: {}", e.what());
        return 1;
    }
    return 0;
}