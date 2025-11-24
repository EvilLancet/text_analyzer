#include "header.hpp"

#include <iostream>
#include <string>
#include <windows.h>
#include <cctype>

using namespace std;

// приветствие и краткая подсказка
static void showWelcome() {
    cout << "Добро пожаловать в программу\n";
    cout << "поиска \"определений\" в тексте\n\n";
    cout << "Программа читает .txt файл (русский текст),\n";
    cout << "находит слова с особыми окончаниями и помечает\n";
    cout << "их символом '±' прямо перед словом.\n";
    cout << "Результат сохраняется в новый файл *_marked.txt\n";
    cout << "в той же папке.\n\n";
}

// запрос пункта меню
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

// простая проверка имени .txt (без пути)
static bool isValidFileName(const string &name) {
    if (name.empty()) return false;

    for (char ch : name) {
        if (ch == '\\' || ch == '/' || ch == ':') return false;
    }

    size_t dotPos = name.find_last_of('.');
    if (dotPos == string::npos) return false;

    string ext = name.substr(dotPos);
    string extLower;
    for (char c : ext)
        extLower.push_back((char)tolower((unsigned char)c));
    return (extLower == ".txt");
}

// формируем имя выходного файла: input.txt -> input_marked.txt
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

// формируем имя файла со статистикой: input.txt -> input_stat.txt
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

// ожидание Enter
static void waitForEnter() {
    cout << "\nНажмите Enter, чтобы продолжить...";
    string dummy;
    getline(cin, dummy);
    cout << "\n";
}

// загрузка файла с окончаниями: фиксированное имя sufix.txt
static bool initSuffixes() {
    const string name = "sufix.txt";

    cout << "Загружается список окончаний из файла \"" << name << "\"...\n";
    cout << "Файл должен быть в кодировке Windows-1251 (ANSI).\n\n";

    if (!loadSuffixesFromFile(name)) {
        cout << "Ошибка: не удалось прочитать окончания из файла \""
             << name << "\".\n";
        cout << "Проверьте, что файл лежит рядом с программой,\n"
                "доступен для чтения и содержит окончания.\n\n";
        return false;
    }

    std::size_t cnt = getSuffixesCount();
    cout << "Окончания успешно загружены.\n";
    cout << "Всего считано окончаний: " << cnt << "\n";

    if (cnt > 0) {
        cout << "Список окончаний, по которым будет идти проверка:\n";
        cout << getSuffixesListString() << "\n\n";
    }

    return true;
}

// обработка выбранного текстового файла
static void handleProcessFile() {
    cout << "\nВведите имя текстового файла из текущей директории.\n";
    cout << "Требования:\n";
    cout << " - файл должен иметь расширение .txt\n";
    cout << " - указывать только имя файла без пути (например: primer.txt)\n";
    cout << " - файл должен лежать рядом с программой\n";
    cout << " - файл должен быть сохранён в кодировке Windows-1251 (ANSI)\n\n";

    cout << "Имя файла: ";
    string fileName;
    getline(cin, fileName);

    if (!isValidFileName(fileName)) {
        cout << "\nОшибка: некорректное имя файла. Пожалуйста, введите что-то вроде text.txt\n";
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

    cout << "\nФайл \"" << fileName << "\" успешно прочитан.\n";
    cout << "Длина текста: " << content.size() << " байт.\n";

    // лексер и пометки
    cout << "\nВыполняется лексический разбор и поиск окончаний...\n";
    Token *tokens = lex(content.c_str());
    mark_tokens_with_endings(tokens);

    // считаем токены и помеченные слова
    int defCount = 0;
    int tokenCount = 0;
    int wordCount  = 0;

    for (Token *cur = tokens; cur != NULL; cur = cur->next) {
        if (cur->type != TOK_EOF)
            tokenCount++;
        if (cur->type == TOK_IDENT)
            wordCount++;
        if (cur->flag)
            defCount++;
    }

    cout << "Всего токенов: " << tokenCount
         << ", слов: " << wordCount
         << ", помечено определений: " << defCount << "\n";

    // формируем текст отчёта по найденным словам
    cout << "\nФормируется отчёт по найденным словам...\n";
    string reportText = buildDefinitionsReport(tokens, defCount);
    string reportFileName = buildStatFileName(fileName);
    if (!writeProcessedTextToFile(reportFileName, reportText)) {
        cout << "\nПредупреждение: не удалось записать отчёт в файл \""
             << reportFileName << "\".\n";
    } else {
        cout << "Отчёт по найденным словам сохранён в файл: "
             << reportFileName << "\n";
    }

    // выводим отчёт сразу в терминал
    cout << "\n----- Отчёт по найденным словам -----\n";
    cout << reportText << "\n";
    cout << "-------------------------------------\n";

    // куда писать размеченный текст
    string outputFileName = buildOutputFileName(fileName);

    cout << "\nВыполняется запись размеченного текста в файл \""
         << outputFileName << "\"...\n";

    // сразу пишем результат в файл (без большой промежуточной строки)
    if (!writeMarkedTextToFile(outputFileName, tokens, content, defCount)) {
        cout << "\nОшибка: не удалось создать файл \""
             << outputFileName << "\" для записи результата.\n";
        free_tokens(tokens);
        waitForEnter();
        return;
    }

    // чистим память
    free_tokens(tokens);

    cout << "\nНайдено определений: " << defCount << "\n";
    cout << "Размеченный текст с нумерацией предложений сохранён в файл: "
         << outputFileName << "\n";

    waitForEnter();
}

int main(int argc, char *argv[]) {
    // кодировка консоли Windows для корректного вывода русского текста и символа '±'
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    (void)argc; (void)argv;

    showWelcome();

    // сначала загружаем окончания из sufix.txt
    if (!initSuffixes()) {
        cout << "\nФайл окончаний не загружен. Завершение работы программы.\n";
        return 0;
    }

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

