export module jizhak.reduction;

import std;

export namespace jzh::reduction {
    inline namespace symbol {
        using wc = wchar_t;
        using u8c = char8_t;
        using u16c = char16_t;
        using u32c = char32_t;
    }

    inline namespace string {
        using str = std::string;
        using wstr = std::wstring;
        using u8str = std::u8string;
        using u16str = std::u16string;
        using u32str = std::u32string;

        using u8s = std::u8string;
        using u16s = std::u16string;
        using u32s = std::u32string;
    }
    inline namespace string_literals {
        constexpr std::string operator""_s(const char* string, size_t size) {
            return std::string(string, size);
        }
        constexpr std::wstring operator""_s(const wchar_t* string, size_t size) {
            return std::wstring(string, size);
        }

        constexpr std::u8string operator""_s(const char8_t* string, size_t size) {
            return std::u8string(string, size);
        }
        constexpr std::u16string operator""_s(const char16_t* string, size_t size) {
            return std::u16string(string, size);
        }
        constexpr std::u32string operator""_s(const char32_t* string, size_t size) {
            return std::u32string(string, size);
        }


        constexpr std::string_view operator""_sv(const char* string, size_t size) {
            return std::string_view(string, size);
        }
        constexpr std::wstring_view operator""_sv(const wchar_t* string, size_t size) {
            return std::wstring_view(string, size);
        }

        constexpr std::u8string_view operator""_sv(const char8_t* string, size_t size) {
            return std::u8string_view(string, size);
        }
        constexpr std::u16string_view operator""_sv(const char16_t* string, size_t size) {
            return std::u16string_view(string, size);
        }
        constexpr std::u32string_view operator""_sv(const char32_t* string, size_t size) {
            return std::u32string_view(string, size);
        }
    }
} // jzh::reduction