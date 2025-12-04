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

// глобальные переменные лексера
static const char *src;
static int pos = 0;

// динамический список суффиксов
static vector<string> g_suffixes;

// ==========================================
// Вспомогательные функции для кодировки CP1251
// ==========================================

// Приведение символа CP1251 к нижнему регистру
static char toLower1251(char c) {
    unsigned char uc = (unsigned char)c;
    // А-Я (0xC0 - 0xDF) -> а-я (0xE0 - 0xFF)
    if (uc >= 0xC0 && uc <= 0xDF) {
        return (char)(uc + 32);
    }
    // Ё (0xA8) -> ё (0xB8)
    if (uc == 0xA8) {
        return (char)0xB8;
    }
    return (char)tolower(uc); // Для латиницы
}

// Проверка: является ли символ русской буквой
static bool isRussianLetter(unsigned char ch) {
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// Проверка: является ли символ частью слова (рус/англ буквы, цифры, _)
static bool isWordChar(unsigned char ch) {
    return isRussianLetter(ch) || isalpha(ch) || isdigit(ch) || ch == '_';
}

// ==========================================
// Логика работы с суффиксами
// ==========================================

static string trim(const string &s) {
    size_t start = 0;
    while (start < s.size() && (unsigned char)s[start] <= 32) ++start;
    size_t end = s.size();
    while (end > start && (unsigned char)s[end - 1] <= 32) --end;
    return s.substr(start, end - start);
}

static void parseSuffixLine(const string &line) {
    string t = trim(line);
    if (t.empty() || t[0] == '#' || (t.size() >= 2 && t[0] == '/' && t[1] == '/')) return;

    size_t curPos = 0;
    while (curPos < t.size()) {
        size_t commaPos = t.find(',', curPos);
        size_t len = (commaPos == string::npos) ? (t.size() - curPos) : (commaPos - curPos);
        string part = trim(t.substr(curPos, len));

        // Удаление кавычек
        if (!part.empty()) {
            if ((part.front() == '\"' && part.back() == '\"') ||
                (part.front() == '\'' && part.back() == '\'')) {
                if (part.size() >= 2) part = part.substr(1, part.size() - 2);
            }
        }

        if (!part.empty()) {
            // Сохраняем суффикс в нижнем регистре для сравнения
            string lowerPart;
            for (char c : part) lowerPart += toLower1251(c);
            g_suffixes.push_back(lowerPart);
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

// Проверка окончания (теперь регистронезависимая)
bool wordHasDefinitionSuffix(const char *word) {
    if (!word || g_suffixes.empty()) return false;
    string w(word);

    // Переводим проверяемое слово в нижний регистр
    string wLower;
    wLower.reserve(w.size());
    for (char c : w) wLower += toLower1251(c);

    for (size_t i = 0; i < g_suffixes.size(); ++i) {
        const string &suf = g_suffixes[i]; // суффиксы уже в lowercase

        if (wLower.size() >= suf.size()) {
            // Сравниваем конец слова
            if (wLower.compare(wLower.size() - suf.size(), suf.size(), suf) == 0) {
                return true;
            }
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

// ==========================================
// Лексер (Разбор текста)
// ==========================================

static void skip_whitespace() {
    while (src[pos] != '\0' && (unsigned char)src[pos] <= 32) pos++;
}

static char *read_identifier() {
    int start = pos;
    while (src[pos] != '\0') {
        unsigned char c = (unsigned char)src[pos];

        if (isWordChar(c)) {
            pos++;
        }
        else if (c == '-') {
            // Дефис внутри слова (кто-то), но не в конце и не в начале
            if (pos > start && isWordChar((unsigned char)src[pos + 1])) {
                pos++;
            } else {
                break;
            }
        }
        else {
            break;
        }
    }

    int length = pos - start;
    if (length == 0) return NULL;

    // Используем new вместо malloc для C++ стиля (хотя для совместимости с struct Token можно оставить malloc)
    // Здесь используем strdup для простоты
    string temp(src + start, length);
    return _strdup(temp.c_str());
}

static char *read_punctuation() {
    int start = pos;

    // Многоточие
    if (src[pos] == '.' && src[pos + 1] == '.' && src[pos + 2] == '.') {
        pos += 3;
        return _strdup("...");
    }

    // Одиночный символ
    if (src[pos] != '\0') {
        char buffer[2] = { src[pos], '\0' };
        pos++;
        return _strdup(buffer);
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

        // 1. Попытка прочитать слово
        if (isWordChar(current)) {
            char *ident = read_identifier();
            if (ident) {
                Token *token = new Token;
                token->type  = TOK_IDENT;
                token->value = ident;
                token->prev  = tail;
                token->next  = NULL;
                token->flag  = false;

                if (!head) head = token;
                else tail->next = token;
                tail = token;
                continue;
            }
        }

        // 2. Иначе читаем как пунктуацию
        char *punct = read_punctuation();
        if (punct) {
            Token *token = new Token;
            token->type  = TOK_PUNCTUATION;
            token->value = punct;
            token->prev  = tail;
            token->next  = NULL;
            token->flag  = false;

            if (!head) head = token;
            else tail->next = token;
            tail = token;
        }
    }

    // Добавляем EOF токен
    Token *eof = new Token;
    eof->type = TOK_EOF;
    eof->value = _strdup("EOF");
    eof->prev = tail;
    eof->next = NULL;
    eof->flag = false;

    if (tail) tail->next = eof;
    else head = eof;

    return head;
}

void mark_tokens_with_endings(Token *head) {
    for (Token *current = head; current != NULL; current = current->next) {
        if (current->type == TOK_IDENT && current->value != NULL) {
            current->flag = wordHasDefinitionSuffix(current->value);
        }
    }
}

void free_tokens(Token *head) {
    Token *cur = head;
    while (cur != NULL) {
        Token *next = cur->next;
        if (cur->value) free(cur->value); // т.к. использовали _strdup
        delete cur;
        cur = next;
    }
}

// ==========================================
// Запись и Форматирование
// ==========================================

bool readTextFromFile(const string &filename, string &outContent) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, ios::end);
    streamoff fileSize = file.tellg();
    if (fileSize < 0) return false;

    outContent.resize((size_t)fileSize);
    file.seekg(0, ios::beg);
    file.read(&outContent[0], fileSize);
    return true;
}

// Проверка на символы конца предложения
static bool isSentenceEnd(const char* val) {
    if (!val) return false;
    return (strcmp(val, ".") == 0 || strcmp(val, "!") == 0 ||
            strcmp(val, "?") == 0 || strcmp(val, "...") == 0);
}

// Функция для определения, нужен ли пробел между токенами
static bool needSpaceBetween(Token* curr, Token* next) {
    if (!curr || !next || next->type == TOK_EOF) return false;

    // 1. Текущий - открывающая скобка: пробел НЕ нужен ("(слово")
    if (curr->type == TOK_PUNCTUATION && strcmp(curr->value, "(") == 0) return false;

    // 2. Следующий - закрывающая скобка: пробел НЕ нужен ("слово)")
    if (next->type == TOK_PUNCTUATION && strcmp(next->value, ")") == 0) return false;

    // 3. Следующий - знаки препинания (.,:;!?): пробел НЕ нужен ("слово.")
    if (next->type == TOK_PUNCTUATION) {
        char c = next->value[0];
        if (c == '.' || c == ',' || c == ':' || c == ';' || c == '!' || c == '?') return false;
    }

    // 4. Текущий - тире (-): пробел НУЖЕН ("слово - слово")
    // Лексер отделяет только "минусы", которые не являются частью слова.
    if (curr->type == TOK_PUNCTUATION && strcmp(curr->value, "-") == 0) return true;

    // 5. Следующий - тире (-): пробел НУЖЕН
    if (next->type == TOK_PUNCTUATION && strcmp(next->value, "-") == 0) return true;

    // 6. Следующий - открывающая скобка: пробел НУЖЕН ("слово (")
    if (next->type == TOK_PUNCTUATION && strcmp(next->value, "(") == 0) return true;

    // 7. По умолчанию, между токенами ставим пробел (Слово Слово, Запятая Слово)
    return true;
}

bool writeMarkedTextToFile(const string &outputFilename,
                           Token *head,
                           const string &originalText,
                           int defCount) {
    ofstream out(outputFilename, ios::binary);
    if (!out.is_open()) return false;

    string hdr = "Найдено определений: " + to_string(defCount) + "\r\n\r\n";
    out.write(hdr.c_str(), hdr.size());

    int sentenceNumber = 1;
    bool startOfSentence = true;
    const string markSymbol = "±";

    Token *curr = head;
    while (curr && curr->type != TOK_EOF) {

        // Нумерация нового предложения
        if (startOfSentence) {
            string numStr = to_string(sentenceNumber) + ") ";
            out.write(numStr.c_str(), numStr.size());
            startOfSentence = false;
        }

        // Вывод самого токена
        if (curr->type == TOK_IDENT) {
            if (curr->flag) out.write("[", 2);
            out.write(curr->value, strlen(curr->value));
            if (curr->flag) out.write("]", 2);
        } else {
            out.write(curr->value, strlen(curr->value));
        }

        // Логика пробелов
        if (needSpaceBetween(curr, curr->next)) {
            out.put(' ');
        }

        // Проверка конца предложения
        if (curr->type == TOK_PUNCTUATION && isSentenceEnd(curr->value)) {
            out.write("\r\n", 2);
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

// ==========================================
// Статистика
// ==========================================

struct DefInfo {
    int count;
    vector<int> sentences;
    DefInfo() : count(0) {}
};

string buildDefinitionsReport(Token *head, int defCount) {
    // map автоматически сортирует ключи по алфавиту
    // Теперь ключ - это оригинальное слово (Окно и окно будут разными)
    map<string, DefInfo> stats;
    int sentenceIndex = 1;

    for (Token *cur = head; cur != NULL; cur = cur->next) {
        // Если это слово и оно помечено (имеет нужное окончание)
        if (cur->type == TOK_IDENT && cur->flag && cur->value != NULL) {
            string word(cur->value); // Берем слово как есть

            DefInfo &info = stats[word]; // Используем оригинальное слово как ключ
            info.count++;

            // Записываем номер предложения, если его еще нет в списке для этого слова
            if (info.sentences.empty() || info.sentences.back() != sentenceIndex) {
                info.sentences.push_back(sentenceIndex);
            }
        }

        // Отслеживаем конец предложения для нумерации
        if (cur->type == TOK_PUNCTUATION && isSentenceEnd(cur->value)) {
            sentenceIndex++;
        }
    }

    ostringstream oss;
    oss << "Всего найдено определений: " << defCount << "\r\n\r\n";

    if (stats.empty()) {
        oss << "Слова не найдены.\r\n";
        return oss.str();
    }

    oss << "Список найденных слов:\r\n";
    int idx = 1;
    for (map<string, DefInfo>::iterator it = stats.begin(); it != stats.end(); ++it) {
        oss << idx++ << ") " << it->first << " — " << it->second.count << " раз(а); предл.: ";
        const vector<int>& sents = it->second.sentences;
        for (size_t i = 0; i < sents.size(); ++i) {
            oss << sents[i] << (i + 1 < sents.size() ? ", " : "");
        }
        oss << "\r\n";
    }
    return oss.str();
}
