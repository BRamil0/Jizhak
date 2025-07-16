import std;
import jizhak;

using namespace std::string_literals;

int main() {
    try {
        jzh::print("Привіт світ!");
    } catch (const std::system_error& e) {
        std::cout << "Error: " << e.what() << '\n';
    }
    return 0;
}