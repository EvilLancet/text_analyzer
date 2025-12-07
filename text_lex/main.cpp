#include "header.hpp"



using namespace std;

// Коды цветов для WinAPI
enum ConsoleColor {
    COLOR_DEFAULT = 7,
    COLOR_BLUE    = 9,
    COLOR_GREEN   = 10,
    COLOR_CYAN    = 11,
    COLOR_RED     = 12,
    COLOR_YELLOW  = 14,
    COLOR_WHITE   = 15
};

// Установка цвета текста в консоли
static void setColor(ConsoleColor color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (WORD)color);
}

// Очистка экрана
inline void clearScreen() {
    system("cls");
}

// Вывод заголовка программы
static void printHeader() {
    setColor(COLOR_CYAN);
    cout << "-------------------------------------------\n";
    cout << "           Поисковик определений           \n";
    cout << "-------------------------------------------\n";
    setColor(COLOR_DEFAULT);
}


// Вывод сообщения об ошибке
static void printError(const string& msg) {
    setColor(COLOR_RED);
    cout << "[Ошибка] " << msg << "\n";
    setColor(COLOR_DEFAULT);
}

// Вывод сообщения об успехе
static void printSuccess(const string& msg) {
    setColor(COLOR_GREEN);
    cout << "[Успешно] " << msg << "\n";
    setColor(COLOR_DEFAULT);
}

// Вывод информационной справки
static void printInfo(const string& msg) {
    setColor(COLOR_YELLOW);
    cout << "[Инфо] " << msg << "\n";
    setColor(COLOR_DEFAULT);
}


// Ожидание нажатия клавиши для продолжения работы
static void waitAnyKey() {
    setColor(COLOR_DEFAULT);
    cout << "\nНажмите любую клавишу, чтобы продолжить...";
    _getch();
}

// Ожидание нажатия перед выходом
static void waitAndExit() {
    setColor(COLOR_WHITE);
    cout << "\nНажмите любую клавишу для выхода...";
    _getch();
}

// Генерация имени выходного файла
static string buildOutputFileName(const string &inputFile) {
    size_t dotPos = inputFile.find_last_of('.');
    if (dotPos != string::npos) {
        string baseName  = inputFile.substr(0, dotPos);
        string extension = inputFile.substr(dotPos);
        return baseName + "_marked" + extension;
    } else {
        return inputFile + "_marked.txt";
    }
}

// Генерация имени файла для отчета по статистике
static string buildStatFileName(const string &inputFile) {
    size_t dotPos = inputFile.find_last_of('.');
    if (dotPos != string::npos) {
        string baseName  = inputFile.substr(0, dotPos);
        string extension = inputFile.substr(dotPos);
        return baseName + "_stat" + extension;
    } else {
        return inputFile + "_stat.txt";
    }
}

// Инициализация: загрузка списка окончаний из файла sufix.txt
static bool initSuffixes() {
    const string name = "sufix.txt";

    // Проверка наличия файла перед попыткой чтения
    DWORD fileAttr = GetFileAttributesA(name.c_str());
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        printError("Файл \"" + name + "\" не найден!");
        cout << "Убедитесь, что файл с окончаниями находится в папке с программой.\n";
        return false;
    }

    // Попытка чтения данных
    if (!loadSuffixesFromFile(name)) {
        printError("Не удалось прочитать окончания из файла \"" + name + "\".");
        return false;
    }

    std::size_t cnt = getSuffixesCount();
    printSuccess("Окончания загружены (" + to_string(cnt) + " шт.)");
    return true;
}

// Сканирование текущей директории на наличие .txt файлов
static vector<string> getTxtFiles() {
    vector<string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("*.txt", &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return files;
    }

    do {
        string fname = findData.cFileName;

        // Фильтрация: исключаем системный файл суффиксов и уже обработанные файлы
        if (fname == "sufix.txt") continue;
        if (fname.find("_marked.txt") != string::npos) continue;
        if (fname.find("_stat.txt") != string::npos) continue;

        files.push_back(fname);
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
    return files;
}

// Обработка выбранного текстового файла

static void handleProcessFile() {
    clearScreen();
    printHeader();
    cout << "\nПоиск текстовых файлов в текущей папке...\n";

    vector<string> files = getTxtFiles();

    if (files.empty()) {
        printInfo("Подходящие файлы .txt не найдены.");
        cout << "Поместите файлы для обработки в папку с программой.\n";
        waitAnyKey();
        return;
    }

    // Вывод списка найденных файлов
    cout << "Найдены файлы для обработки:\n\n";
    for (size_t i = 0; i < files.size(); ++i) {
        setColor(COLOR_WHITE);
        cout << " " << (i + 1) << ". ";
        setColor(COLOR_GREEN);
        cout << files[i] << "\n";
    }
    setColor(COLOR_DEFAULT);

    int choice = 0;

    // --- НАЧАЛО ИЗМЕНЕНИЙ ---
    while (true) {
        cout << "\nВведите номер файла (или 0 для отмены): ";

        if (!(cin >> choice)) {
            cin.clear();
            string dummy; getline(cin, dummy);
            printError("Некорректный ввод.");
            // waitAnyKey(); // Убрали, чтобы сразу можно было вводить снова
            continue; // Возвращаемся в начало цикла
        }
        // Очищаем буфер после ввода числа
        string dummy; getline(cin, dummy);

        if (choice == 0) return;

        if (choice < 1 || choice > (int)files.size()) {
            printError("Неверный номер файла.");
            // waitAnyKey();
            continue; // Возвращаемся в начало цикла
        }

        break; // Ввод корректен, выходим из цикла while
    }
    // --- КОНЕЦ ИЗМЕНЕНИЙ ---

    string fileName = files[choice - 1];

    cout << "\n-------------------------------------------\n";
    printInfo("Чтение файла: " + fileName);

    // читаем исходный текст
    string content;
    if (!readTextFromFile(fileName, content)) {
        printError("Не удалось открыть \"" + fileName + "\".");
        waitAnyKey();
        return;
    }

    // лексер и пометки
    cout << "Лексический разбор и поиск определений...\n";
    Token *tokens = lex(content.c_str());
    mark_tokens_with_endings(tokens);

    // считаем статистику
    int defCount = 0;
    for (Token *cur = tokens; cur != NULL; cur = cur->next) {
        if (cur->flag) defCount++;
    }

     // Формирование и сохранение файла с отчетом
    string reportFileName = buildStatFileName(fileName);
    string reportText = buildDefinitionsReport(tokens, defCount);

    if (writeProcessedTextToFile(reportFileName, reportText)) {
        printSuccess("Статистика сохранена: " + reportFileName);
    } else {
        printError("Ошибка записи статистики.");
    }

    // Вывод краткого отчета на экран
    setColor(COLOR_CYAN);
    cout << "\n--- Найдено определений: " << defCount << " ---\n";
    setColor(COLOR_DEFAULT);

    // Сохранение размеченного текста
    string outputFileName = buildOutputFileName(fileName);
    if (writeMarkedTextToFile(outputFileName, tokens, content, defCount)) {
        printSuccess("Результат сохранён: " + outputFileName);
    } else {
        printError("Ошибка записи результата.");
    }

    // Очистка памяти списка токенов
    free_tokens(tokens);
    waitAnyKey();
}

// Главное меню
static void showMainMenu() {
    while (true) {
        clearScreen();
        printHeader();

        cout << "\nВыберите действие:\n\n";
        cout << " [1] Обработать файл (выбрать из списка)\n";
        cout << " [2] О программе / Помощь\n";
        cout << " [3] Выход\n";

        char choice = _getch();
        cout << choice << "\n";

        switch (choice) {
            case '1':
                handleProcessFile();
                break;
            case '2':
                clearScreen();
                printHeader();
                cout << "\nПрограмма читает .txt файл, находит\n";
                cout << "определения с окончаниями из sufix.txt\n";
                cout << "и помечает их символами [ и ].\n\n";
                cout << "Требования:\n";
                cout << " - Кодировка файлов: Windows-1251\n";
                cout << " - Файлы должны лежать рядом с .exe\n";
                waitAnyKey();
                break;
            case '3':
                return; // Возврат в main для корректного завершения
            default:
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    // Настройка консоли
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    clearScreen();
    printHeader();

    // Загрузка суффиксов при старте
    cout << "\nИнициализация...\n";
    if (!initSuffixes()) {
        cout << "\nКритическая ошибка. Работа невозможна.\n";
        waitAndExit();
        return 0;
    }

    // Небольшая пауза, чтобы пользователь увидел статус загрузки
    Sleep(1500);

    // Запуск цикла меню
    showMainMenu();

    cout << "\nЗавершение работы. До свидания!\n";
    Sleep(1000);
    return 0;
}
