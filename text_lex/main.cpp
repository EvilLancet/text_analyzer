#include "header.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <cctype>
#include <conio.h>
#include <algorithm> // для transform

using namespace std;

// ==========================================
// Утилиты для работы с консолью (Цвета и UI)
// ==========================================

enum ConsoleColor {
    COLOR_DEFAULT = 7,
    COLOR_BLUE    = 9,
    COLOR_GREEN   = 10,
    COLOR_CYAN    = 11,
    COLOR_RED     = 12,
    COLOR_YELLOW  = 14,
    COLOR_WHITE   = 15
};

static void setColor(ConsoleColor color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (WORD)color);
}

static void clearScreen() {
    system("cls");
}

static void printHeader() {
    setColor(COLOR_CYAN);
    cout << "-------------------------------------------\n";
    cout << "           Поисковик определений           \n";
    cout << "-------------------------------------------\n";
    setColor(COLOR_DEFAULT);
}

static void printError(const string& msg) {
    setColor(COLOR_RED);
    cout << "[Ошибка] " << msg << "\n";
    setColor(COLOR_DEFAULT);
}

static void printSuccess(const string& msg) {
    setColor(COLOR_GREEN);
    cout << "[Успешно] " << msg << "\n";
    setColor(COLOR_DEFAULT);
}

static void printInfo(const string& msg) {
    setColor(COLOR_YELLOW);
    cout << "[Инфо] " << msg << "\n";
    setColor(COLOR_DEFAULT);
}

// ==========================================
// Основная логика
// ==========================================

// Функция ожидания нажатия клавиши перед выходом
static void waitAnyKey() {
    setColor(COLOR_DEFAULT);
    cout << "\nНажмите любую клавишу, чтобы продолжить...";
    _getch();
}

static void waitAndExit() {
    setColor(COLOR_WHITE);
    cout << "\nНажмите любую клавишу для выхода...";
    _getch();
}

// Формируем имя выходного файла: input.txt -> input_marked.txt
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

// Формируем имя файла со статистикой: input.txt -> input_stat.txt
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

// Загрузка файла с окончаниями
static bool initSuffixes() {
    const string name = "sufix.txt";

    // Проверка наличия файла перед попыткой чтения
    DWORD fileAttr = GetFileAttributesA(name.c_str());
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        printError("Файл \"" + name + "\" не найден!");
        cout << "Убедитесь, что файл с окончаниями находится в папке с программой.\n";
        return false;
    }

    if (!loadSuffixesFromFile(name)) {
        printError("Не удалось прочитать окончания из файла \"" + name + "\".");
        return false;
    }

    std::size_t cnt = getSuffixesCount();
    printSuccess("Окончания загружены (" + to_string(cnt) + " шт.)");
    return true;
}

// Получение списка .txt файлов в текущей директории
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
        // чтобы не засорять список
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

    cout << "Найдены файлы для обработки:\n\n";
    for (size_t i = 0; i < files.size(); ++i) {
        setColor(COLOR_WHITE);
        cout << " " << (i + 1) << ". ";
        setColor(COLOR_GREEN);
        cout << files[i] << "\n";
    }
    setColor(COLOR_DEFAULT);

    cout << "\nВведите номер файла (или 0 для отмены): ";
    int choice = 0;
    if (!(cin >> choice)) {
        cin.clear();
        string dummy; getline(cin, dummy);
        printError("Некорректный ввод.");
        waitAnyKey();
        return;
    }
    // Очищаем буфер после ввода числа
    string dummy; getline(cin, dummy);

    if (choice == 0) return;

    if (choice < 1 || choice > (int)files.size()) {
        printError("Неверный номер файла.");
        waitAnyKey();
        return;
    }

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

    // Сохранение отчета
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

    free_tokens(tokens);
    waitAnyKey();
}

// Главное меню
static void showMainMenu() {
    while (true) {
        clearScreen();
        printHeader();

        // Выводим статус, если нужно (например, суффиксы загружены)
        // Здесь можно добавить проверку статуса, если потребуется

        cout << "\nВыберите действие:\n\n";
        cout << " [1] Обработать файл (выбрать из списка)\n";
        cout << " [2] О программе / Помощь\n";
        cout << " [3] Выход\n";
        cout << "\nВаш выбор: ";

        char choice = _getch();
        cout << choice << "\n"; // Эхо нажатой клавиши

        switch (choice) {
            case '1':
                handleProcessFile();
                break;
            case '2':
                clearScreen();
                printHeader();
                cout << "\nПрограмма читает .txt файл (русский текст),\n";
                cout << "находит слова с окончаниями из sufix.txt и помечает\n";
                cout << "их символом '±'.\n\n";
                cout << "Требования:\n";
                cout << " - Кодировка файлов: Windows-1251 (ANSI)\n";
                cout << " - Файлы должны лежать рядом с .exe\n";
                waitAnyKey();
                break;
            case '3':
                return; // Выход из showMainMenu, попадаем в main и на выход
            default:
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    // Настройка консоли
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    // Пытаемся скрыть мигающий курсор для красоты (опционально)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = true; // Оставим видимым для ввода
    SetConsoleCursorInfo(hConsole, &cursorInfo);

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
