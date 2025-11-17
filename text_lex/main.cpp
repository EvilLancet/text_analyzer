#include "header.hpp"

#include <iostream>
#include <string>
#include <windows.h>
#include <cctype>

using namespace std;

// Выводит приветственное сообщение и краткую справку.
static void showWelcome() {
    cout << "Добро пожаловать в программу\n";
    cout << "поиска \"определений\" в тексте\n\n";
    cout << "Эта программа читает .txt файл (русский текст),\n";
    cout << "находит слова с особыми окончаниями и помечает\n";
    cout << "их символом '±' прямо перед словом.\n";
    cout << "Результат сохраняется в новый файл *_marked.txt\n";
    cout << "в той же папке.\n\n";
}

// Запрашивает у пользователя выбор в главном меню (1 или 2). Повторяет запрос при неверном вводе.
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

// Проверяет корректность имени файла (.txt, без пути, не пустое).
static bool isValidFileName(const string &name) {
    if (name.empty()) return false;

    for (char ch : name) {
        if (ch == '\\' || ch == '/' || ch == ':') {
            // запрещено указывать путь или недопустимый символ
            return false;
        }
    }

    size_t dotPos = name.find_last_of('.');
    if (dotPos == string::npos) return false;

    string ext = name.substr(dotPos);
    string extLower;
    for (char c : ext) extLower.push_back((char)tolower((unsigned char)c));
    return (extLower == ".txt");
}

// Формирует имя выходного файла, добавляя суффикс _marked ("input.txt" -> "input_marked.txt").
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

// Ожидает нажатия Enter перед продолжением.
static void waitForEnter() {
    cout << "\nНажмите Enter, чтобы продолжить...";
    string dummy;
    getline(cin, dummy);
    cout << "\n";
}

// Обрабатывает выбранный файл: читает текст, отмечает "определения" и сохраняет результат.
static void handleProcessFile() {
    cout << "\nВведите имя текстового файла из текущей директории.\n";
    cout << "Требования:\n";
    cout << " - файл должен иметь расширение .txt\n";
    cout << " - указывать только имя файла без пути (например: primer.txt)\n";
    cout << " - файл должен лежать рядом с программой\n\n";

    cout << "Имя файла: ";
    string fileName;
    getline(cin, fileName);

    if (!isValidFileName(fileName)) {
        cout << "\nОшибка: некорректное имя файла. Пожалуйста, введите что-то вроде text.txt\n";
        waitForEnter();
        return;
    }

    // Читаем исходный текст
    string content;
    if (!readTextFromFile(fileName, content)) {
        cout << "\nОшибка: не удалось открыть \"" << fileName << "\".\n";
        cout << "Проверьте, что файл существует и доступен для чтения.\n";
        waitForEnter();
        return;
    }

    // Разбиваем текст на токены и помечаем "определения"
    Token *tokens = lex(content.c_str());
    mark_tokens_with_endings(tokens);
    int defCount = 0;
    for (Token *cur = tokens; cur != NULL; cur = cur->next) {
        if (cur->flag) defCount++;
    }

    // Собираем финальный текст с символом '±'
    string outputText = assembleTextFromTokens(tokens, content);
    string prefix = string("Найдено определений: ") + to_string(defCount) + "\r\n";
    outputText = prefix + outputText;

    // Формируем имя файла с результатом
    string outputFileName = buildOutputFileName(fileName);

    // Записываем результат в файл
    if (!writeProcessedTextToFile(outputFileName, outputText)) {
        cout << "\nОшибка: не удалось создать файл \"" << outputFileName << "\" для записи результата.\n";
        free_tokens(tokens);
        waitForEnter();
        return;
    }

    // Освобождаем память токенов
    free_tokens(tokens);
    cout << "\nНайдено определений: " << defCount << "\n";
    cout << "\nГотово!\n";
    cout << "Результат сохранён в файл: " << outputFileName << "\n";

    waitForEnter();
}

int main(int argc, char *argv[]) {
    // Кодировка консоли Windows для корректного вывода русского текста и символа '±'.
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    (void)argc; (void)argv; // гасим предупреждения о неиспользуемых параметрах

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
