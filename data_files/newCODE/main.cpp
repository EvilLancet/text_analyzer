// main.cpp
// Консольный интерфейс для пользователя.
//
// Поведение по умолчанию (запуск без аргументов):
// 1. Печатаем приветствие и краткую справку.
// 2. Показываем меню:
//      [1] Обработать текстовый файл (.txt) из текущей директории
//      [2] Выход
//    (разрешены только варианты 1 или 2)
// 3. Если выбран пункт 1:
//      - спрашиваем имя файла (только имя без пути);
//      - проверяем, что это *.txt и что файл лежит рядом с программой;
//      - запускаем анализ текста и создаём файл "<имя>_marked.txt";
//      - ждём нажатия Enter и возвращаемся в меню.
// 4. Если выбран пункт 2 — программа завершает работу.
//
// Если программа запущена с аргументом "-gui" (регистр неважен),
// вместо консольного меню вызывается runGUI(), где используется стандартный
// диалог выбора файла Windows (WinAPI).

#include "header.hpp"

#include <iostream>
#include <string>
#include <windows.h>
#include <limits>
#include <cctype>

using namespace std;

// Печатает приветствие и краткую справку о том, что делает программа.
static void showWelcome() {
    cout << "Доброе пожаловать в программу\n";
    cout << "поиска \"определений\" в тексте\n";
    cout << "\n\n";
    cout << "Эта программа читает .txt файл (русский текст),\n";
    cout << "находит слова с особыми окончаниями и помечает\n";
    cout << "их символом '±' прямо перед словом.\n";
    cout << "Результат сохраняется в новый файл *_marked.txt\n";
    cout << "в той же папке.\n\n";
}

// Считывает выбор пользователя в главном меню.
// Возвращает 1 или 2. Пока не будет введено корректное значение,
// функция повторяет запрос.
static int getMenuChoice() {
    while (true) {
        cout << "Главное меню:\n";
        cout << " 1 - Обработать текстовый файл (.txt) из текущей директории\n";
        cout << " 2 - Выход из программы\n";
        cout << "Ваш выбор (1/2): ";

        string choice;
        getline(cin, choice);

        if (choice == "1") return 1;
        if (choice == "2") return 2;

        cout << "\nНекорректный ввод. Пожалуйста, введите только 1 или 2.\n\n";
    }
}

// Проверяет корректность имени файла.
// Условия:
//  - имя не пустое;
//  - имя не содержит путей / слэшей / двоеточий;
//  - расширение .txt (регистр не важен).
// Возвращает true, если имя допустимо.
static bool isValidFileName(const string &name) {
    if (name.empty()) return false;

    for (char ch : name) {
        if (ch == '\\' || ch == '/' || ch == ':') {
            // запрещаем путь, допускается только само имя файла
            return false;
        }
    }

    size_t dotPos = name.find_last_of('.');
    if (dotPos == string::npos) return false;

    string ext = name.substr(dotPos); // включая точку
    string extLower;
    extLower.reserve(ext.size());
    for (char c : ext) {
        extLower.push_back((char)tolower((unsigned char)c));
    }

    return (extLower == ".txt");
}

// Формирует имя результирующего файла по имени исходного.
// Пример: "input.txt" -> "input_marked.txt".
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

// Небольшая пауза, чтобы пользователь успел прочитать сообщение.
// Ждём нажатия Enter и возвращаемся в меню.
static void waitForEnter() {
    cout << "\nНажмите Enter, чтобы вернуться в меню...";
    string dummy;
    getline(cin, dummy);
    cout << "\n";
}

// Полный цикл обработки файла, выбранного пользователем:
// 1) спрашивает имя файла;
// 2) проверяет корректность имени;
// 3) читает исходный текст из этого файла;
// 4) лексер делит текст на токены и помечает "определения";
// 5) собирает обработанный текст, добавляя символ '±' перед помеченными словами;
// 6) сохраняет результат в <имя>_marked.txt;
// 7) ждёт Enter.
static void handleProcessFile() {
    cout << "\nВведите имя текстового файла из ТЕКУЩЕЙ директории.\n";
    cout << "Требования:\n";
    cout << " - файл должен иметь расширение .txt\n";
    cout << " - указывать только имя файла без пути (например: primer.txt)\n";
    cout << " - файл должен лежать рядом с программой\n\n";

    cout << "Имя файла: ";
    string fileName;
    getline(cin, fileName);

    if (!isValidFileName(fileName)) {
        cout << "\nОшибка: некорректное имя файла. "
             << "Пожалуйста, введите что-то вроде text.txt\n";
        waitForEnter();
        return;
    }

    // читаем исходный текст
    string content;
    if (!readTextFromFile(fileName, content)) {
        cout << "\nОшибка: не удалось открыть \"" << fileName << "\".\n";
        cout << "Проверьте, что файл существует и доступен для чтения.\n";
        waitForEnter();
        return;
    }

    // разбиение текста на токены и пометка "определений"
    Token *tokens = lex(content.c_str());
    mark_tokens_with_endings(tokens);

    // сборка финального текста с символом '±'
    string outputText = assembleTextFromTokens(tokens, content);

    // формируем имя файла с результатом
    string outputFileName = buildOutputFileName(fileName);

    // запись результата в файл
    if (!writeProcessedTextToFile(outputFileName, outputText)) {
        cout << "\nОшибка: не удалось создать файл \"" << outputFileName
             << "\" для записи результата.\n";
        free_tokens(tokens);
        waitForEnter();
        return;
    }

    // освобождаем память токенов
    free_tokens(tokens);

    cout << "\nГотово ?\n";
    cout << "Результат сохранён в файл: " << outputFileName << "\n";

    waitForEnter();
}

// Проверяет, просил ли пользователь GUI-режим.
// Возвращает true, если среди аргументов командной строки есть "-gui"
// (регистр неважен).
static bool wantsGUI(int argc, char *argv[]) {
    if (argc <= 1) return false;

    string arg = argv[1];
    for (auto &c : arg) {
        c = (char)tolower((unsigned char)c);
    }
    return (arg == "-gui");
}

// Точка входа.
// Шаги:
// 1) Настраиваем кодовые страницы консоли Windows, чтобы русские символы
//    и символ '±' отображались корректно.
// 2) Если передан ключ -gui, запускаем графический режим (runGUI())
//    и завершаем программу.
// 3) Иначе запускается обычное консольное меню.
int main(int argc, char *argv[]) {
    // Настройка кодовой страницы для корректного вывода в консоль Windows.
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    // Если пользователь явно просит GUI-режим:
    if (wantsGUI(argc, argv)) {
        runGUI();
        return 0;
    }

    // Обычный консольный режим с меню
    showWelcome();

    while (true) {
        int choice = getMenuChoice();

        if (choice == 1) {
            handleProcessFile();
        } else if (choice == 2) {
            cout << "Завершение работы. До свидания!\n";
            break;
        }
    }

    return 0;
}
