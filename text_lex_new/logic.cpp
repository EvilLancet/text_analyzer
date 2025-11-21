#include "header.hpp"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fstream>
#include <string>
using namespace std;

// глобальные переменные лексера: вход и позици€
static const char *src;
static int pos = 0;

// список суффиксов дл€ пометки "определений"
static char a[370][4] = {
    "йс€", "мс€", "его", "шей", "ого", "ес€", "шим", "ших", "ему", "ной", "ос€",
    "хс€", "ому", "ным", "ных", "шее", "шем", "ший", "шие", "щей", "юс€", "ус€",
    "ими", "мой", "ном", "щим", "щих", "ное", "ные", "ный", "ыми", "мым", "мых",
    "шую", "ша€", "шею", "щие", "щем", "щее", "щий", "ис€", "€с€", "мом", "мое",
    "мые", "мый", "ною", "на€", "ную", "щую", "щею", "ща€", "мую", "ма€", "мою",
    "той", "емо", "ема", "емы", "тым", "тых", "ано", "ана", "аны", "ено", "ена",
    "ены", "тые", "тое", "тый", "ван", "тую", "та€", "тою", "чен", "лен", "чна",
    "чны", "чно", "нно", "нна", "нны", "ово", "имо", "нен", "рен", "имы", "има",
    "ьей", "€ем", "жен", "тен", "тны", "тна", "тно", "ьим", "ьих", "уто", "ута",
    "уты", "ато", "рна", "рны", "рно", "лЄн", "ден", "шен", "иво", "сто", "ива",
    "ивы", "дно", "дна", "дны", "тан", "ино", "чий", "кан", "нЄн", "пан", "чЄн",
    "€но", "чьи", "ито", "чье", "ндо", "жна", "жны", "жно", "ьею", "сен", "ево",
    "рЄн", "жЄн", "сна", "сны", "сно", "рым", "рых", "йно", "йна", "йны", "щЄн",
    "-то", "щен", "оен", "ган", "ват", "мна", "мны", "мно", "зан", "сан", "тто",
    "ран", "рые", "дан", "рый", "сым", "сых", "шЄн", "чью", "дЄн", "шна", "шны",
    "шно", "еен", "жий", "ибо", "удь", "рто", "ыто", "тич", "ыта", "сые", "сое",
    "ыты", "€то", "€ты", "сый", "зых", "зым", "тЄн", "шан", "егл", "тыж", "етх",
    "€зо", "€хл", "дюж", "бущ", "тхл", "€дл", "вущ",
    "аов", "Єло", "ижн", "дащ", "лыс", "окр", "едл", "хуч", "Єгл", "ыхл", "€бо",
    "лощ", "уЄн", "Єзо", "тощ", "суч", "с€€", "сюю", "юно", "цно", "рюч", "рзо",
    "вещ", "орл", "€чо", "€бл", "упл", "одл", "туч", "юто", "мо€", "агл", "хоч",
    "скл", "ежо", "жЄл", "уим", "ыро", "€гл", "Єдр", "€ро"
};

// русска€ буква? (Windows-1251)
static int is_russian_letter(int ch) {
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// знак препинани€ дл€ отдельного токена
static int is_punctuation(int ch) {
    return ch == ',' || ch == '.' || ch == '!' || ch == '?' ||
           ch == ';' || ch == ':' || ch == '-' || ch == '(' ||
           ch == ')' || ch == '\"' || ch == '\'' || ch == 'Е';
}

// пропуск пробелов
static void skip_whitespace() {
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) pos++;
}

// читаем идентификатор (слово). ¬ј∆Ќќ: дефис не входит в слово.
static char *read_identifier() {
    int start = pos;
    while (src[pos] != '\0' && (is_russian_letter((unsigned char)src[pos]) ||
                                isalpha((unsigned char)src[pos]) ||
                                src[pos] == '_')) {
        pos++;
    }
    int length = pos - start;
    if (length == 0) return NULL;
    char *ident = (char*)malloc(length + 1);
    strncpy(ident, &src[start], length);
    ident[length] = '\0';
    return ident;
}

// читаем пунктуацию (многоточие как один токен)
static char *read_punctuation() {
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

// лексер: разбивает входную строку на токены
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
                Token *token = (Token*)malloc(sizeof(Token));
                token->type  = TOK_IDENT;
                token->value = strdup(ident);
                token->prev  = NULL;
                token->next  = NULL;
                token->flag  = false;

                if (head == NULL) { head = tail = token; }
                else { tail->next = token; token->prev = tail; tail = token; }

                free(ident);
            }
            continue;
        } else if (is_punctuation((unsigned char)current)) {
            char *punct = read_punctuation();
            if (punct != NULL) {
                Token *token = (Token*)malloc(sizeof(Token));
                token->type  = TOK_PUNCTUATION;
                token->value = strdup(punct);
                token->prev  = NULL;
                token->next  = NULL;
                token->flag  = false;

                if (head == NULL) { head = tail = token; }
                else { tail->next = token; token->prev = tail; tail = token; }

                free(punct);
            }
            continue;
        } else {
            char symbol[2] = { current, '\0' };
            Token *token = (Token*)malloc(sizeof(Token));
            token->type  = TOK_PUNCTUATION;
            token->value = strdup(symbol);
            token->prev  = NULL;
            token->next  = NULL;
            token->flag  = false;

            if (head == NULL) { head = tail = token; }
            else { tail->next = token; token->prev = tail; tail = token; }

            pos++;
        }
    }
    // служебный конец
    Token *eof = (Token*)malloc(sizeof(Token));
    eof->type  = TOK_EOF;
    eof->value = strdup("EOF");
    eof->prev  = tail;
    eof->next  = NULL;
    eof->flag  = false;
    if (tail) tail->next = eof; else head = eof;

    return head;
}

// помечаем слова-токены по суффиксам
// дополнительно игнорируем слова, если вдруг внутри есть дефис
void mark_tokens_with_endings(Token *head) {
    for (Token *current = head; current != NULL; current = current->next) {
        if (current->type != TOK_IDENT || current->value == NULL) continue;

        if (strchr(current->value, '-') != NULL) {          // защита от "что-то"
            current->flag = false;
            continue;
        }
        size_t len = strlen(current->value);
        if (len < 3) { current->flag = false; continue; }

        char last_three[4];
        strncpy(last_three, current->value + len - 3, 3);
        last_three[3] = '\0';

        bool found = false;
        for (int i = 0; i < 370; ++i) {
            if (strcmp(last_three, a[i]) == 0) { found = true; break; }
        }
        current->flag = found;
    }
}

// освободить всю пам€ть списка токенов
void free_tokens(Token *head) {
    for (Token *cur = head; cur != NULL; ) {
        Token *next = cur->next;
        free(cur->value);
        free(cur);
        cur = next;
    }
}

// читает файл целиком в строку (двоичное чтение)
bool readTextFromFile(const string &filename, string &outContent) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) return false;
    file.seekg(0, ios::end);
    streamoff fileSize = file.tellg();
    if (fileSize < 0) fileSize = 0;
    outContent.resize((size_t)fileSize);
    file.seekg(0, ios::beg);
    file.read(&outContent[0], fileSize);
    file.close();
    return true;
}

// сразу пишем помеченный текст в файл (без промежуточной строки)
bool writeMarkedTextToFile(const string &outputFilename,
                           Token *head,
                           const string &originalText,
                           int defCount) {
    ofstream out(outputFilename, ios::binary);
    if (!out.is_open()) return false;

    // шапка
    string hdr = string("Ќайдено определений: ") + to_string(defCount) + "\r\n";
    out.write(hdr.c_str(), hdr.size());

    Token *currentToken = head;
    string currentWord;

    // идЄм по исходному тексту
    for (size_t i = 0; i < originalText.size(); ++i) {
        unsigned char c = (unsigned char)originalText[i];

        if (is_russian_letter(c) || isalpha(c) || c == '_') {
            // собираем слово
            currentWord.push_back((char)c);
        } else {
            // закончили слово перед небуквенным символом
            if (!currentWord.empty()) {
                while (currentToken && currentToken->type != TOK_IDENT)
                    currentToken = currentToken->next;

                if (currentToken && currentToken->type == TOK_IDENT) {
                    if (currentToken->flag) out.put('±');
                    out.write(currentToken->value, (std::streamsize)strlen(currentToken->value));
                    currentToken = currentToken->next;
                } else {
                    out.write(currentWord.c_str(), (std::streamsize)currentWord.size());
                }
                currentWord.clear();
            }
            // сам символ переносим как есть
            out.put((char)c);
        }
    }

    // хвост, если текст закончилс€ на букве
    if (!currentWord.empty()) {
        while (currentToken && currentToken->type != TOK_IDENT)
            currentToken = currentToken->next;

        if (currentToken && currentToken->type == TOK_IDENT) {
            if (currentToken->flag) out.put('±');
            out.write(currentToken->value, (std::streamsize)strlen(currentToken->value));
            currentToken = currentToken->next;
        } else {
            out.write(currentWord.c_str(), (std::streamsize)currentWord.size());
        }
    }

    out.close();
    return true;
}

// запись произвольной строки в файл
bool writeProcessedTextToFile(const string &outputFilename, const string &text) {
    ofstream outFile(outputFilename, ios::binary);
    if (!outFile.is_open()) return false;
    outFile.write(text.c_str(), (std::streamsize)text.size());
    outFile.close();
    return true;
}
