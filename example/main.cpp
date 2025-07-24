import std;
import jizhak;

using namespace std::string_literals;

int main() {
    auto tpm = jzh::new_tpm(4);
    try {
        tpm->add_task([] {
            jzh::print("Привіт світ!");
        });
    } catch (const std::system_error& e) {
        std::cout << "Error: " << e.what() << '\n';
    }
    tpm->wait_all();
    return 0;
}