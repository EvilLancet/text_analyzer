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

// динамический список суффиксов (окончаний), считываемый из файла
static vector<string> g_suffixes;

// простое обрезание пробелов в начале и конце строки
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

// разбор одной строки с суффиксами
// поддерживает форматы:
//   "ась", "тся", "енн"
//   а также просто: ась
static void parseSuffixLine(const string &line) {
    string t = trim(line);

    // пустые строки пропускаем
    if (t.empty())
        return;

    // комментарии: строки, начинающиеся с # или //
    if (t[0] == '#')
        return;
    if (t.size() >= 2 && t[0] == '/' && t[1] == '/')
        return;

    // делим строку по запятой, чтобы поддерживать формат
    // "ась", "тся", "енн"
    size_t pos = 0;
    while (pos < t.size()) {
        size_t commaPos = t.find(',', pos);
        size_t len = (commaPos == string::npos) ? (t.size() - pos) : (commaPos - pos);
        string part = trim(t.substr(pos, len));

        // убираем кавычки вокруг суффикса, если есть
        if (!part.empty()) {
            if ((part.front() == '\"' && part.back() == '\"') ||
                (part.front() == '\'' && part.back() == '\'')) {
                if (part.size() >= 2)
                    part = part.substr(1, part.size() - 2);
            }
        }

        // пропускаем пустой результат
        if (!part.empty()) {
            // суффиксы с дефисом (типа "-то") нам не нужны
            if (part.find('-') == string::npos) {
                g_suffixes.push_back(part);
            }
        }

        if (commaPos == string::npos)
            break;

        pos = commaPos + 1;
    }
}

// загрузка окончаний из текстового файла
bool loadSuffixesFromFile(const string &filename) {
    g_suffixes.clear();

    ifstream in(filename.c_str(), ios::binary);
    if (!in.is_open()) {
        return false;
    }

    string line;
    while (getline(in, line)) {
        parseSuffixLine(line);
    }

    // важно: никаких sort/unique — количество суффиксов должно
    // совпадать с тем, что в файле (например, 370).
    return !g_suffixes.empty();
}

// проверка: заканчивается ли слово на одно из считанных окончаний
bool wordHasDefinitionSuffix(const char *word) {
    if (!word || g_suffixes.empty())
        return false;

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

// количество загруженных суффиксов (для вывода в терминал)
std::size_t getSuffixesCount() {
    return g_suffixes.size();
}

// список суффиксов одной строкой (для вывода в терминал)
std::string getSuffixesListString() {
    ostringstream oss;
    for (size_t i = 0; i < g_suffixes.size(); ++i) {
        oss << g_suffixes[i];
        if (i + 1 < g_suffixes.size())
            oss << ", ";
    }
    return oss.str();
}

// русская буква? (Windows-1251)
static int is_russian_letter(int ch) {
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// знак препинания для отдельного токена
static int is_punctuation(int ch) {
    return ch == ',' || ch == '.' || ch == '!' || ch == '?' ||
           ch == ';' || ch == ':' || ch == '-' || ch == '(' ||
           ch == ')' || ch == '\"' || ch == '\'' || ch == 'Е';
}

// пропуск пробелов
static void skip_whitespace() {
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) pos++;
}

// читаем идентификатор (слово). Важно: дефис не входит в слово.
static char *read_identifier() {
    int start = pos;
    while (src[pos] != '\0' &&
           (is_russian_letter((unsigned char)src[pos]) ||
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
        if (is_russian_letter((unsigned char)current) ||
            isalpha((unsigned char)current)) {

            char *ident = read_identifier();
            if (ident != NULL) {
                Token *token = (Token*)malloc(sizeof(Token));
                token->type  = TOK_IDENT;
                token->value = _strdup(ident);
                token->prev  = NULL;
                token->next  = NULL;
                token->flag  = false;

                if (head == NULL) {
                    head = tail = token;
                } else {
                    tail->next = token;
                    token->prev = tail;
                    tail = token;
                }

                free(ident);
            }
            continue;

        } else if (is_punctuation((unsigned char)current)) {
            char *punct = read_punctuation();
            if (punct != NULL) {
                Token *token = (Token*)malloc(sizeof(Token));
                token->type  = TOK_PUNCTUATION;
                token->value = _strdup(punct);
                token->prev  = NULL;
                token->next  = NULL;
                token->flag  = false;

                if (head == NULL) {
                    head = tail = token;
                } else {
                    tail->next = token;
                    token->prev = tail;
                    tail = token;
                }

                free(punct);
            }
            continue;

        } else {
            // всё, что не буква и не "нормальная" пунктуация — отдельный токен
            char symbol[2] = { current, '\0' };
            Token *token = (Token*)malloc(sizeof(Token));
            token->type  = TOK_PUNCTUATION;
            token->value = _strdup(symbol);
            token->prev  = NULL;
            token->next  = NULL;
            token->flag  = false;

            if (head == NULL) {
                head = tail = token;
            } else {
                tail->next = token;
                token->prev = tail;
                tail = token;
            }

            pos++;
        }
    }

    // служебный конец
    Token *eof = (Token*)malloc(sizeof(Token));
    eof->type  = TOK_EOF;
    eof->value = _strdup("EOF");
    eof->prev  = tail;
    eof->next  = NULL;
    eof->flag  = false;
    if (tail) tail->next = eof; else head = eof;

    return head;
}

// помечаем слова-токены по окончаниям
// дополнительно игнорируем слова, если внутри есть дефис
void mark_tokens_with_endings(Token *head) {
    for (Token *current = head; current != NULL; current = current->next) {
        if (current->type != TOK_IDENT || current->value == NULL) continue;

        // защита от "что-то" и т.п.
        if (strchr(current->value, '-') != NULL) {
            current->flag = false;
            continue;
        }

        current->flag = wordHasDefinitionSuffix(current->value);
    }
}

// освободить всю память списка токенов
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
// + нумерация предложений прямо в тексте: 1), 2), 3) ...
bool writeMarkedTextToFile(const string &outputFilename,
                           Token *head,
                           const string &originalText,
                           int defCount) {
    ofstream out(outputFilename, ios::binary);
    if (!out.is_open()) return false;

    // шапка
    string hdr = string("Найдено определений: ") + to_string(defCount) + "\r\n";
    out.write(hdr.c_str(), hdr.size());

    Token *currentToken = head;
    string currentWord;

    int  sentenceNumber   = 1;  // номер текущего предложения
    bool atSentenceStart  = true; // флаг "мы в начале предложения"

    // идём по исходному тексту
    for (size_t i = 0; i < originalText.size(); ++i) {
        unsigned char c = (unsigned char)originalText[i];

        // если мы в начале предложения, то ждём первый "не пробельный" символ
        // и перед ним выводим номер: 1), 2), ...
        if (atSentenceStart) {
            if (!isspace(c)) {
                ostringstream tmp;
                tmp << sentenceNumber << ") ";
                string numStr = tmp.str();
                out.write(numStr.c_str(), (std::streamsize)numStr.size());
                atSentenceStart = false;
            }
        }

        if (is_russian_letter(c) || isalpha(c) || c == '_') {
            // собираем слово
            currentWord.push_back((char)c);
        } else {
            // закончили слово перед небуквенным символом
            if (!currentWord.empty()) {
                while (currentToken && currentToken->type != TOK_IDENT)
                    currentToken = currentToken->next;

                if (currentToken && currentToken->type == TOK_IDENT) {
                    if (currentToken->flag) out.put('[');
                    out.write(currentToken->value,
                              (std::streamsize)strlen(currentToken->value));
                    if (currentToken->flag) out.put(']');
                    currentToken = currentToken->next;
                } else {
                    out.write(currentWord.c_str(),
                              (std::streamsize)currentWord.size());
                }
                currentWord.clear();
            }

            // обработка знаков препинания (в т.ч. многоточия)
            if (c == '.' &&
                i + 2 < originalText.size() &&
                originalText[i + 1] == '.' &&
                originalText[i + 2] == '.') {

                // многоточие: выводим как есть и считаем концом предложения
                out.write("...", 3);
                i += 2; // ещё две точки "съели"

                sentenceNumber++;
                atSentenceStart = true;
            } else {
                // обычный одиночный символ
                out.put((char)c);

                // конец предложения по . ! ?
                if (c == '.' || c == '!' || c == '?') {
                    sentenceNumber++;
                    atSentenceStart = true;
                }
            }
        }
    }

    // хвост, если текст закончился на букве
    if (!currentWord.empty()) {
        while (currentToken && currentToken->type != TOK_IDENT)
            currentToken = currentToken->next;

        if (currentToken && currentToken->type == TOK_IDENT) {
            if (currentToken->flag) out.put('[');
            out.write(currentToken->value,
                      (std::streamsize)strlen(currentToken->value));
            if (currentToken->flag) out.put(']');
            currentToken = currentToken->next;
        } else {
            out.write(currentWord.c_str(),
                      (std::streamsize)currentWord.size());
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

// структура для статистики по одному слову
struct DefInfo {
    int count;               // сколько раз слово было "определением"
    vector<int> sentences;   // номера предложений, где оно встречается

    DefInfo() : count(0) {}
};

// проверка, что токен завершает предложение
static bool isSentenceTerminatorToken(const Token *t) {
    if (!t) return false;
    if (t->type != TOK_PUNCTUATION || t->value == NULL) return false;

    return strcmp(t->value, ".")   == 0 ||
           strcmp(t->value, "!")   == 0 ||
           strcmp(t->value, "?")   == 0 ||
           strcmp(t->value, "...") == 0;
}

// формирует текст отчёта по всем найденным "определениям"
string buildDefinitionsReport(Token *head, int defCount) {
    map<string, DefInfo> stats;

    int sentenceIndex = 1; // нумерация предложений с 1

    for (Token *cur = head; cur != NULL; cur = cur->next) {
        // обновляем номер предложения
        if (isSentenceTerminatorToken(cur)) {
            sentenceIndex++;
        }

        // интересуют только токены-слова, помеченные как "определение"
        if (cur->type == TOK_IDENT && cur->flag && cur->value != NULL) {
            string word(cur->value);

            DefInfo &info = stats[word];
            info.count++;

            // чтобы один и тот же номер предложения не добавлять по нескольку раз
            if (info.sentences.empty() || info.sentences.back() != sentenceIndex) {
                info.sentences.push_back(sentenceIndex);
            }
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
    for (map<string, DefInfo>::const_iterator it = stats.begin();
         it != stats.end(); ++it) {
        const string &word = it->first;
        const DefInfo &info = it->second;

        oss << idx++ << ") " << word
            << " — всего " << info.count << " раз(а); предложения: ";

        for (size_t i = 0; i < info.sentences.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << info.sentences[i];
        }
        oss << "\r\n";
    }

    return oss.str();
}
