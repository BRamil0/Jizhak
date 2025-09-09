## Їжак (Jižak)

__[English](docs/README.en.md)__

#### Опис
Це невелика бібліотека на C++23 яка надає різні інструменти.

### Особливості
- Написано з використанням надсучасних можливостей C++23.
- Повне використання модулів.
- Повна підтримка Unicode.
- Універсальність.

### Залежності
- CMake 4.0.3 або новіше.
- Компілятор з підтримкою C++23 (Clang 20+ або MSVC 14.44+).
- Git.
- Boost та fmt.
- Опціонально: пакетний менеджер (Conan або vcpkg).

### Встановлення

Рекомендується використовувати автоматичне встановлення через систему збірки (CMake):
```bash
git clone https://github.com/BRamil0/Jizhak.git
cd Jizhak
cmake -B build -S .
cmake --build build
```

Для ручного встановлення дивитися файл [manual_installation](docs/manual_installation.uk.md) (тимчасово у процесі).

###  Використання
Просто імпортуйте модуль jizhak.

```c++
import jizhak;
```

### Приклади використання
Дивиться файл [example.cpp](example/main_example.cpp).

### Автоматична документація Doxyfile
1. Встановить Doxyfile.
2. Виконайте в корні проєкту:
    ```bash
    doxygen Doxyfile
    ```
3. Перегляньте файл [index.html](docs/doxygen/html/index.html) за шляхом `/docs/doxygen/html/index.html`.

### Ліцензія
Цей проєкт ліцензовано за ліцензією **MIT**. Детальніше дивіться у файлі [LICENSE](LICENSE.txt).