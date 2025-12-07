#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>
#include <iostream>
#include <vector>
#include <windows.h>
#include <cctype>
#include <conio.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fstream>
#include <map>
#include <sstream>



// типы токенов
enum TokenT {
    TOK_EOF,          // конец потока
    TOK_IDENT,        // слово
    TOK_PUNCTUATION,  // знаки препинания и прочие одиночные символы
};

// структура токена
struct Token {
    TokenT type;
    bool   flag;   // флаг - слово определение
    char  *value;
    Token *prev;
    Token *next;
};

// Лексический разбор и пометки
Token *lex(const char *input);
void   mark_tokens_with_endings(Token *head);
void   free_tokens(Token *head);

// Работа со списком окончаний
bool loadSuffixesFromFile(const std::string &filename); // чтение окончаний из файла
bool wordHasDefinitionSuffix(const char *word);         // проверка слова по окончаниям

// Вспомогательные функции для отладки суффиксов
std::size_t getSuffixesCount();                 // количество загруженных суффиксов
std::string getSuffixesListString();           // все суффиксы в одной строке

// Построение текстового отчёта по найденным словам
std::string buildDefinitionsReport(Token *head, int defCount);

// Работа с файлами
bool readTextFromFile(const std::string &filename, std::string &outContent);

// Пишет размеченный текст в файл
bool writeMarkedTextToFile(const std::string &outputFilename,
                           Token *head,
                           const std::string &originalText,
                           int defCount);

// запись произвольной строки text в файл
bool writeProcessedTextToFile(const std::string &outputFilename, const std::string &text);

#endif // HEADER_HPP
