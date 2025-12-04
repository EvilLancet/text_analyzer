#include "header.hpp"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

using namespace std;

// глобальные переменные лексера: вход и позиция
static const char *src;
static int pos = 0;

// динамический список суффиксов
static vector<string> g_suffixes;

// простое обрезание пробелов
static string trim(const string &s) {
    size_t start = 0;
    while (start < s.size() &&
           (s[start] == ' ' || s[start] == '\t' || s[start] == '\r'))
        ++start;

    size_t end = s.size();
    while (end > start &&
           (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r'))
        --end;

    return s.substr(start, end - start);
}

// разбор строки с суффиксами
static void parseSuffixLine(const string &line) {
    string t = trim(line);
    if (t.empty()) return;
    if (t[0] == '#') return;
    if (t.size() >= 2 && t[0] == '/' && t[1] == '/') return;

    size_t curPos = 0;
    while (curPos < t.size()) {
        size_t commaPos = t.find(',', curPos);
        size_t len = (commaPos == string::npos) ? (t.size() - curPos) : (commaPos - curPos);
        string part = trim(t.substr(curPos, len));

        if (!part.empty()) {
            if ((part.front() == '\"' && part.back() == '\"') ||
                (part.front() == '\'' && part.back() == '\'')) {
                if (part.size() >= 2)
                    part = part.substr(1, part.size() - 2);
            }
        }

        if (!part.empty()) {
            // Разрешаем суффиксы даже с дефисом, если вдруг они есть в файле
            g_suffixes.push_back(part);
        }

        if (commaPos == string::npos) break;
        curPos = commaPos + 1;
    }
}

bool loadSuffixesFromFile(const string &filename) {
    g_suffixes.clear();
    ifstream in(filename.c_str(), ios::binary);
    if (!in.is_open()) return false;
    string line;
    while (getline(in, line)) parseSuffixLine(line);
    return !g_suffixes.empty();
}

bool wordHasDefinitionSuffix(const char *word) {
    if (!word || g_suffixes.empty()) return false;
    string w(word);

    for (size_t i = 0; i < g_suffixes.size(); ++i) {
        const string &suf = g_suffixes[i];
        if (w.size() >= suf.size() &&
            w.compare(w.size() - suf.size(), suf.size(), suf) == 0) {
            return true;
        }
    }
    return false;
}

std::size_t getSuffixesCount() { return g_suffixes.size(); }

std::string getSuffixesListString() {
    ostringstream oss;
    for (size_t i = 0; i < g_suffixes.size(); ++i) {
        oss << g_suffixes[i];
        if (i + 1 < g_suffixes.size()) oss << ", ";
    }
    return oss.str();
}

// Русская буква (CP1251)
static int is_russian_letter(int ch) {
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// Является ли символ частью слова (буква, цифра, подчеркивание)
// Дефис обрабатывается отдельно в read_identifier
static bool is_word_char(int ch) {
    return is_russian_letter(ch) || isalpha(ch) || ch == '_' || isdigit(ch);
}

static int is_punctuation(int ch) {
    return ch == ',' || ch == '.' || ch == '!' || ch == '?' ||
           ch == ';' || ch == ':' || ch == '-' || ch == '(' ||
           ch == ')' || ch == '\"' || ch == '\'' || ch == '\n' || ch == '\r';
}

static void skip_whitespace() {
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) pos++;
}

// --- ИЗМЕНЕНИЕ 1: Чтение слова с поддержкой дефисов внутри (кто-то, по-русски) ---
static char *read_identifier() {
    int start = pos;

    while (src[pos] != '\0') {
        unsigned char c = (unsigned char)src[pos];

        if (is_word_char(c)) {
            pos++;
        }
        else if (c == '-') {
            // Дефис считаем частью слова ТОЛЬКО если он внутри слова:
            // Слева у нас уже есть символы (pos > start),
            // а справа должна быть буква.
            // Иначе это тире или минус.
            if (pos > start && is_word_char((unsigned char)src[pos + 1])) {
                pos++;
            } else {
                // Это дефис в конце слова или тире -- прерываем слово
                break;
            }
        }
        else {
            break;
        }
    }

    int length = pos - start;
    if (length == 0) return NULL;
    char *ident = (char*)malloc(length + 1);
    strncpy(ident, &src[start], length);
    ident[length] = '\0';
    return ident;
}

static char *read_punctuation() {
    int start = pos;
    // Многоточие
    if (src[pos] == '.' && src[pos + 1] == '.' && src[pos + 2] == '.') {
        pos += 3;
        char *punct = (char*)malloc(4);
        strncpy(punct, &src[start], 3);
        punct[3] = '\0';
        return punct;
    }

    // Одиночная пунктуация (включая одиночный дефис/тире, если он не попал в слово)
    // Любой символ, который не буква и не пробел, считаем пунктуацией для сохранения структуры
    if (src[pos] != '\0') {
        pos++;
        char *punct = (char*)malloc(2);
        punct[0] = src[start];
        punct[1] = '\0';
        return punct;
    }
    return NULL;
}

Token *lex(const char *input) {
    src = input;
    pos = 0;
    Token *head = NULL;
    Token *tail = NULL;

    while (src[pos] != '\0') {
        skip_whitespace();
        if (src[pos] == '\0') break;

        unsigned char current = (unsigned char)src[pos];

        // Пробуем прочитать как слово
        if (is_word_char(current)) {
            char *ident = read_identifier();
            if (ident) {
                Token *token = (Token*)malloc(sizeof(Token));
                token->type  = TOK_IDENT;
                token->value = _strdup(ident);
                token->prev  = NULL; token->next = NULL; token->flag = false;
                if (!head) head = tail = token;
                else { tail->next = token; token->prev = tail; tail = token; }
                free(ident);
                continue;
            }
        }

        // Если не слово, читаем как пунктуацию
        char *punct = read_punctuation();
        if (punct) {
            Token *token = (Token*)malloc(sizeof(Token));
            token->type  = TOK_PUNCTUATION;
            token->value = _strdup(punct);
            token->prev  = NULL; token->next = NULL; token->flag = false;
            if (!head) head = tail = token;
            else { tail->next = token; token->prev = tail; tail = token; }
            free(punct);
        }
    }

    Token *eof = (Token*)malloc(sizeof(Token));
    eof->type = TOK_EOF; eof->value = _strdup("EOF");
    eof->prev = tail; eof->next = NULL; eof->flag = false;
    if (tail) tail->next = eof; else head = eof;
    return head;
}

void mark_tokens_with_endings(Token *head) {
    for (Token *current = head; current != NULL; current = current->next) {
        if (current->type != TOK_IDENT || current->value == NULL) continue;

        // --- ИЗМЕНЕНИЕ: Убрана проверка, игнорирующая слова с дефисом.
        // Теперь "какой-то" будет проверяться на окончание "то".

        current->flag = wordHasDefinitionSuffix(current->value);
    }
}

void free_tokens(Token *head) {
    for (Token *cur = head; cur != NULL; ) {
        Token *next = cur->next;
        free(cur->value);
        free(cur);
        cur = next;
    }
}

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

// --- ИЗМЕНЕНИЕ 2 и 3: Вывод предложений с новой строки и использование символа '±' ---
bool writeMarkedTextToFile(const string &outputFilename,
                           Token *head,
                           const string &originalText, // Больше не используется для сборки, берем токены
                           int defCount) {
    ofstream out(outputFilename, ios::binary);
    if (!out.is_open()) return false;

    // Шапка
    string hdr = string("Найдено определений: ") + to_string(defCount) + "\r\n\r\n";
    out.write(hdr.c_str(), hdr.size());

    int sentenceNumber = 1;
    bool startOfSentence = true;

    // Символ пометки
    const string markSymbol = "±";

    Token *curr = head;
    while (curr && curr->type != TOK_EOF) {

        // 1. Обработка начала предложения (нумерация)
        if (startOfSentence) {
            // Пропускаем пробелы в начале предложения (токены с пробелами лексером не сохраняются,
            // но нам и не нужно сохранять форматирование пробелов между предложениями, раз мы делаем new line)

            // Записываем номер: "1) "
            ostringstream oss;
            oss << sentenceNumber << ") ";
            string numStr = oss.str();
            out.write(numStr.c_str(), numStr.size());
            startOfSentence = false;
        }

        // 2. Вывод токена
        if (curr->type == TOK_IDENT) {
            // Если помечено - ставим ±
            if (curr->flag) {
                out.write(markSymbol.c_str(), markSymbol.size());
            }
            out.write(curr->value, strlen(curr->value));
        } else {
            // Пунктуация
            out.write(curr->value, strlen(curr->value));
        }

        // 3. Логика пробелов
        // Лексер съел пробелы. Нам нужно восстановить пробел после слова,
        // если следующий токен - это слово или открывающая скобка/кавычка.
        // Если следующий токен - запятая или точка, пробел не нужен.
        if (curr->next && curr->next->type != TOK_EOF) {
            Token *next = curr->next;
            bool needSpace = false;

            // Простейшая эвристика: нужен пробел, если следом идет слово
            if (next->type == TOK_IDENT) {
                needSpace = true;
                // Исключение: открывающая скобка перед словом не требует пробела перед собой?
                // Нет, обычно "слово (слово)".
            }
            // Если следом пунктуация, пробел нужен только если это тире, открывающая скобка/кавычка
            else if (next->type == TOK_PUNCTUATION) {
                char c = next->value[0];
                if (c == '-' && strlen(next->value) == 1) needSpace = true; // тире
                if (c == '(' || c == '\"' || c == '\'') {
                     // С пробелами вокруг кавычек сложно без контекста, но обычно перед открывающей нужен пробел.
                     // В рамках простой лабы ставим пробел.
                     if (curr->type != TOK_PUNCTUATION || strcmp(curr->value, "(") != 0)
                        needSpace = true;
                }
            }

            // Не ставим пробел перед точкой, запятой, двоеточием, закрывающей скобкой
            if (next->type == TOK_PUNCTUATION) {
                char c = next->value[0];
                if (c == '.' || c == ',' || c == '!' || c == '?' || c == ':' || c == ';' || c == ')')
                    needSpace = false;
            }

            if (needSpace) {
                out.put(' ');
            }
        }

        // 4. Проверка на конец предложения
        bool isEnd = false;
        if (curr->type == TOK_PUNCTUATION) {
            if (strcmp(curr->value, ".") == 0 ||
                strcmp(curr->value, "!") == 0 ||
                strcmp(curr->value, "?") == 0 ||
                strcmp(curr->value, "...") == 0) {
                isEnd = true;
            }
        }

        if (isEnd) {
            out.write("\r\n", 2); // Новая строка
            sentenceNumber++;
            startOfSentence = true;
        }

        curr = curr->next;
    }

    out.close();
    return true;
}

bool writeProcessedTextToFile(const string &outputFilename, const string &text) {
    ofstream outFile(outputFilename, ios::binary);
    if (!outFile.is_open()) return false;
    outFile.write(text.c_str(), text.size());
    outFile.close();
    return true;
}

struct DefInfo {
    int count;
    vector<int> sentences;
    DefInfo() : count(0) {}
};

static bool isSentenceTerminatorToken(const Token *t) {
    if (!t || t->type != TOK_PUNCTUATION) return false;
    return strcmp(t->value, ".") == 0 ||
           strcmp(t->value, "!") == 0 ||
           strcmp(t->value, "?") == 0 ||
           strcmp(t->value, "...") == 0;
}

string buildDefinitionsReport(Token *head, int defCount) {
    map<string, DefInfo> stats;
    int sentenceIndex = 1;

    for (Token *cur = head; cur != NULL; cur = cur->next) {
        if (cur->type == TOK_IDENT && cur->flag && cur->value != NULL) {
            string word(cur->value);
            DefInfo &info = stats[word];
            info.count++;
            if (info.sentences.empty() || info.sentences.back() != sentenceIndex) {
                info.sentences.push_back(sentenceIndex);
            }
        }
        // Увеличиваем счетчик ПОСЛЕ обработки слова, если это конец
        if (isSentenceTerminatorToken(cur)) {
            sentenceIndex++;
        }
    }

    ostringstream oss;
    oss << "Всего найдено определений: " << defCount << "\r\n\r\n";

    if (stats.empty()) {
        oss << "Слова, являющиеся искомым членом предложения, не найдены.\r\n";
        return oss.str();
    }

    oss << "Список слов, являющихся искомым членом предложения:\r\n";
    int idx = 1;
    for (map<string, DefInfo>::const_iterator it = stats.begin(); it != stats.end(); ++it) {
        const string &word = it->first;
        const DefInfo &info = it->second;
        oss << idx++ << ") " << word << " — всего " << info.count << " раз(а); предложения: ";
        for (size_t i = 0; i < info.sentences.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << info.sentences[i];
        }
        oss << "\r\n";
    }
    return oss.str();
}
