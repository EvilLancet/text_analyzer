#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>

// общий заголовок проекта "поиск определений".
// здесь объявления того, чем пользуются остальные файлы.

// перечисление типов токенов
enum TokenT {
    TOK_EOF,          // конец потока
    TOK_IDENT,        // слово
    TOK_PUNCTUATION,  // знаки препинания и прочие одиночные символы
    TOK_SPACE         // пробелы (не используется)
};

// структура токена (нужна в main.cpp для доступа к полям)
struct Token {
    TokenT type;
    bool   flag;   // пометка "определение"
    char  *value;
    Token *prev;
    Token *next;
};

// лексический разбор и пометки
Token *lex(const char *input);
void   mark_tokens_with_endings(Token *head);
void   free_tokens(Token *head);

// работа с файлами
bool readTextFromFile(const std::string &filename, std::string &outContent);

// сразу пишет помеченный текст в файл (без большой промежуточной строки)
bool writeMarkedTextToFile(const std::string &outputFilename,
                           Token *head,
                           const std::string &originalText,
                           int defCount);

// запись произвольной строки text в файл
bool writeProcessedTextToFile(const std::string &outputFilename, const std::string &text);

#endif // HEADER_HPP
