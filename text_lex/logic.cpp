#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fstream>
#include <string>
using namespace std;

// Глобальные переменные лексера: входная строка и текущая позиция
static const char *src;
static int pos = 0;

// Массив суффиксов для идентификации "определений"
static char a[370][4] = {
    "йся", "мся", "его", "шей", "ого", "еся", "шим", "ших", "ему", "ной", "ося",
    "хся", "ому", "ным", "ных", "шее", "шем", "ший", "шие", "щей", "юся", "уся",
    "ими", "мой", "ном", "щим", "щих", "ное", "ные", "ный", "ыми", "мым", "мых",
    "шую", "шая", "шею", "щие", "щем", "щее", "щий", "ися", "яся", "мом", "мое",
    "мые", "мый", "ною", "ная", "ную", "щую", "щею", "щая", "мую", "мая", "мою",
    "той", "емо", "ема", "емы", "тым", "тых", "ано", "ана", "аны", "ено", "ена",
    "ены", "тые", "тое", "тый", "ван", "тую", "тая", "тою", "чен", "лен", "чна",
    "чны", "чно", "нно", "нна", "нны", "ово", "имо", "нен", "рен", "имы", "има",
    "ьей", "яем", "жен", "тен", "тны", "тна", "тно", "ьим", "ьих", "уто", "ута",
    "уты", "ато", "рна", "рны", "рно", "лён", "ден", "шен", "иво", "сто", "ива",
    "ивы", "дно", "дна", "дны", "тан", "ино", "чий", "кан", "нён", "пан", "чён",
    "яно", "чьи", "ито", "чье", "ндо", "жна", "жны", "жно", "ьею", "сен", "ево",
    "рён", "жён", "сна", "сны", "сно", "рым", "рых", "йно", "йна", "йны", "щён",
    "-то", "щен", "оен", "ган", "ват", "мна", "мны", "мно", "зан", "сан", "тто",
    "ран", "рые", "дан", "рый", "сым", "сых", "шён", "чью", "дён", "шна", "шны",
    "шно", "еен", "жий", "ибо", "удь", "рто", "ыто", "тич", "ыта", "сые", "сое",
    "ыты", "ято", "яты", "сый", "зых", "зым", "тён", "шан", "егл", "тыж", "етх",
    "язо", "яхл", "дюж", "бущ", "тхл", "ядл", "вущ",
    "аов", "ёло", "ижн", "дащ", "лыс", "окр", "едл", "хуч", "ёгл", "ыхл", "ябо",
    "лощ", "уён", "ёзо", "тощ", "суч", "сяя", "сюю", "юно", "цно", "рюч", "рзо",
    "вещ", "орл", "ячо", "ябл", "упл", "одл", "туч", "юто", "моя", "агл", "хоч",
    "скл", "ежо", "жёл", "уим", "ыро", "ягл", "ёдр", "яро"
};

// Перечисление типов токенов
typedef enum {
    TOK_EOF,         // Конец входного потока
    TOK_IDENT,       // Идентификатор (слово)
    TOK_PUNCTUATION, // Знаки препинания (и прочие отдельные символы)
    TOK_SPACE        // Пробельные символы (не используется)
} TokenT;

typedef struct Token {
    TokenT type;
    bool flag;       // Пометка "определение" (true, если слово оканчивается одним из указанных суффиксов)
    char *value;
    struct Token *prev;
    struct Token *next;
} Token;

// Создает новый токен указанного типа, копируя переданное значение
Token *create_token(TokenT type, const char *value) {
    Token *token = (Token*)malloc(sizeof(Token));
    token->type = type;
    token->value = strdup(value);
    token->prev = NULL;
    token->next = NULL;
    token->flag = false;
    return token;
}

// Добавляет токен в конец двусвязного списка токенов
void add_token(Token **head, Token **tail, Token *token) {
    if (*head == NULL) {
        // Если список пуст, новый токен становится первым элементом
        *head = token;
        *tail = token;
    } else {
        // Иначе добавляем в конец списка
        (*tail)->next = token;
        token->prev = *tail;
        *tail = token;
    }
}

// Освобождает память всего списка токенов
void free_tokens(Token *head) {
    Token *current = head;
    while (current != NULL) {
        Token *next = current->next;
        free(current->value);
        free(current);
        current = next;
    }
}

// Проверяет, является ли символ русской буквой (кодировка Windows-1251)
int is_russian_letter(int ch) {
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// Проверяет, является ли символ знаком препинания для отдельного токена
int is_punctuation(int ch) {
    return ch == ',' || ch == '.' || ch == '!' || ch == '?' ||
           ch == ';' || ch == ':' || ch == '-' || ch == '(' ||
           ch == ')' || ch == '\"' || ch == '\'' || ch == '…';
}

// Пропускает пробельные символы во входной строке
void skip_whitespace() {
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) {
        pos++;
    }
}

// Читает идентификатор (последовательность букв, '_' или '-') из входного текста, возвращает новую строку (char*)
char *read_identifier() {
    int start = pos;
    while (src[pos] != '\0' && (is_russian_letter((unsigned char)src[pos]) ||
                                isalpha((unsigned char)src[pos]) ||
                                src[pos] == '_' ||
                                src[pos] == '-')) {
        pos++;
    }
    int length = pos - start;
    if (length == 0) return NULL;
    char *ident = (char*)malloc(length + 1);
    strncpy(ident, &src[start], length);
    ident[length] = '\0';
    return ident;
}

// Читает символ(ы) пунктуации (три точки как один токен). Возвращает новую строку (char*)
char *read_punctuation() {
    int start = pos;
    if (src[pos] == '.' && src[pos + 1] == '.' && src[pos + 2] == '.') {
        pos += 3;
        char *punct = (char*)malloc(4);
        strncpy(punct, &src[start], 3);
        punct[3] = '\0';
        return punct;
    }
    if (is_punctuation((unsigned char)src[pos])) {
        pos++;
        char *punct = (char*)malloc(2);
        punct[0] = src[start];
        punct[1] = '\0';
        return punct;
    }
    return NULL;
}

// Разбивает входную строку на токены и возвращает список токенов
Token *lex(const char *input) {
    src = input;
    pos = 0;

    Token *head = NULL;
    Token *tail = NULL;
    while (src[pos] != '\0') {
        skip_whitespace();
        if (src[pos] == '\0') break;

        char current = src[pos];
        if (is_russian_letter((unsigned char)current) || isalpha((unsigned char)current)) {
            char *ident = read_identifier();
            if (ident != NULL) {
                Token *token = create_token(TOK_IDENT, ident);
                add_token(&head, &tail, token);
                free(ident);
            }
            continue;
        } else if (is_punctuation((unsigned char)current)) {
            char *punct = read_punctuation();
            if (punct != NULL) {
                Token *token = create_token(TOK_PUNCTUATION, punct);
                add_token(&head, &tail, token);
                free(punct);
            }
            continue;
        } else {
            char symbol[2] = { current, '\0' };
            Token *token = create_token(TOK_PUNCTUATION, symbol);
            add_token(&head, &tail, token);
            pos++;
        }
    }
    Token *eof = create_token(TOK_EOF, "EOF");
    add_token(&head, &tail, eof);
    return head;
}

// Помечает токены-слова, оканчивающиеся на заданные суффиксы
void mark_tokens_with_endings(Token *head) {
    Token *current = head;
    while (current != NULL) {
        if (current->type == TOK_IDENT && current->value != NULL) {
            size_t len = strlen(current->value);
            if (len >= 3) {
                char last_three[4];
                strncpy(last_three, current->value + len - 3, 3);
                last_three[3] = '\0';
                bool found = false;
                for (int i = 0; i < 370; i++) {
                    if (strcmp(last_three, a[i]) == 0) {
                        found = true;
                        break;
                    }
                }
                current->flag = found;
            } else {
                current->flag = false;
            }
        }
        current = current->next;
    }
}

// Читает весь файл filename в строку outContent (Windows-1251). Возвращает true при успехе, false при ошибке.
bool readTextFromFile(const string &filename, string &outContent) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.seekg(0, ios::end);
    streamoff fileSize = file.tellg();
    if (fileSize < 0) fileSize = 0;
    outContent.resize((size_t)fileSize);
    file.seekg(0, ios::beg);
    file.read(&outContent[0], fileSize);
    file.close();
    return true;
}

// Собирает текст из списка токенов, добавляя '±' перед отмеченными словами. Возвращает получившийся текст.
string assembleTextFromTokens(Token *head, const string &originalText) {
    string result;
   
    Token *currentToken = head;
    string currentWord = "";

    // Проходим по каждому символу исходного текста
    for (size_t i = 0; i < originalText.size(); ++i) {
        unsigned char c = (unsigned char) originalText[i];
        if (is_russian_letter(c) || isalpha(c) || c == '_' || c == '-') {
            // Строим текущее слово
            currentWord.push_back((char)c);
        } else {
            // Встретился символ, не принадлежащий слову
            if (!currentWord.empty()) {
                // Завершили текущее слово перед символом c
                while (currentToken != NULL && currentToken->type != TOK_IDENT) {
                    currentToken = currentToken->next;
                }
                if (currentToken != NULL && currentToken->type == TOK_IDENT) {
                    if (currentToken->flag) {
                        result.push_back('±');
                    }
                    result += currentToken->value;
                    currentToken = currentToken->next;
                } else {
                    result += currentWord;
                }
                currentWord.clear();
            }
            // Добавляем текущий не-буквенный символ
            result.push_back((char)c);
        }
    }
    // Обрабатываем остаток, если текст заканчивался на букву
    if (!currentWord.empty()) {
        while (currentToken != NULL && currentToken->type != TOK_IDENT) {
            currentToken = currentToken->next;
        }
        if (currentToken != NULL && currentToken->type == TOK_IDENT) {
            if (currentToken->flag) {
                result.push_back('±');
            }
            result += currentToken->value;
            currentToken = currentToken->next;
        } else {
            result += currentWord;
        }
        currentWord.clear();
    }
    return result;
}

// Записывает обработанный текст в файл outputFilename. Возвращает true при успехе, false при ошибке.
bool writeProcessedTextToFile(const string &outputFilename, const string &text) {
    ofstream outFile(outputFilename, ios::binary);
    if (!outFile.is_open()) {
        return false;
    }
    outFile.write(text.c_str(), text.size());
    outFile.close();
    return true;
}
