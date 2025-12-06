#include "header.hpp"

using namespace std;

// глобальные переменные для парсинга
static const char *src;
static int pos = 0;

// список суффиксов, который грузим из файла
static vector<string> g_suffixes;

// функция чтобы сделать буквы маленькими (для cp1251)
static char toLower1251(char c) {
    unsigned char uc = (unsigned char)c;
    if (uc >= 0xC0 && uc <= 0xDF) {
        return (char)(uc + 32);
    }
    if (uc == 0xA8) {
        return (char)0xB8;
    }
    return (char)tolower(uc);
}

// проверяем, русская это буква или нет
static bool isRussianLetter(unsigned char ch) {
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// проверка символа: буква, цифра или подчеркивание
static bool isWordChar(unsigned char ch) {
    return isRussianLetter(ch) || isalpha(ch) || isdigit(ch) || ch == '_';
}

static string trim(const string &s) {
    size_t start = 0;
    while (start < s.size() && (unsigned char)s[start] <= 32) ++start;
    size_t end = s.size();
    while (end > start && (unsigned char)s[end - 1] <= 32) --end;
    return s.substr(start, end - start);
}

// читаем суффиксы из файла и сохраняем в вектор
bool loadSuffixesFromFile(const string &filename) {
    g_suffixes.clear();
    ifstream in(filename.c_str());

    if (!in.is_open()) return false;

    string word;
    while (in >> word) {
        string lowerPart;
        lowerPart.reserve(word.size());

        // сразу переводим в нижний регистр
        for (char c : word) {
            lowerPart += toLower1251(c);
        }
        g_suffixes.push_back(lowerPart);
    }
    return !g_suffixes.empty();
}

// проверяем подходит ли слово под наши суффиксы
bool wordHasDefinitionSuffix(const char *word) {
    if (!word || g_suffixes.empty()) return false;
    string w(word);

    string wLower;
    wLower.reserve(w.size());
    for (char c : w) wLower += toLower1251(c);

    // пробегаемся по всем загруженным суффиксам
    for (size_t i = 0; i < g_suffixes.size(); ++i) {
        const string &suf = g_suffixes[i];

        if (wLower.size() >= suf.size()) {
            if (wLower.compare(wLower.size() - suf.size(), suf.size(), suf) == 0) {
                return true;
            }
        }
    }
    return false;
}

std::size_t getSuffixesCount() { return g_suffixes.size(); }

// собираем все суффиксы в одну строку для вывода
std::string getSuffixesListString() {
    ostringstream oss;
    for (size_t i = 0; i < g_suffixes.size(); ++i) {
        oss << g_suffixes[i];
        if (i + 1 < g_suffixes.size()) oss << ", ";
    }
    return oss.str();
}

static void skip_whitespace() {
    while (src[pos] != '\0' && (unsigned char)src[pos] <= 32) pos++;
}

// считываем слово целиком из строки
static char *read_identifier() {
    int start = pos;
    while (src[pos] != '\0') {
        unsigned char c = (unsigned char)src[pos];

        if (isWordChar(c)) {
            pos++;
        }
        else if (c == '-') {
            // если дефис внутри слова, то берем его
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

    string temp(src + start, length);
    return _strdup(temp.c_str());
}

// обработка знаков препинания
static char *read_punctuation() {
    int start = pos;

    if (src[pos] == '.' && src[pos + 1] == '.' && src[pos + 2] == '.') {
        pos += 3;
        return _strdup("...");
    }

    if (src[pos] != '\0') {
        char buffer[2] = { src[pos], '\0' };
        pos++;
        return _strdup(buffer);
    }
    return NULL;
}

// основной цикл разбора текста на токены
Token *lex(const char *input) {
    src = input;
    pos = 0;
    Token *head = NULL;
    Token *tail = NULL;

    while (src[pos] != '\0') {
        skip_whitespace();
        if (src[pos] == '\0') break;

        unsigned char current = (unsigned char)src[pos];

        // если это буква, пробуем считать слово
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

        // иначе это знак препинания
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

    // добавляем токен конца файла
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

// проходим по списку и ставим флаг, если окончание подходит
void mark_tokens_with_endings(Token *head) {
    for (Token *current = head; current != NULL; current = current->next) {
        if (current->type == TOK_IDENT && current->value != NULL) {
            current->flag = wordHasDefinitionSuffix(current->value);
        }
    }
}

// очищаем память списка токенов
void free_tokens(Token *head) {
    Token *cur = head;
    while (cur != NULL) {
        Token *next = cur->next;
        if (cur->value) free(cur->value);
        delete cur;
        cur = next;
    }
}

// просто читаем весь файл в строку
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

static bool isSentenceEnd(const char* val) {
    if (!val) return false;
    return (strcmp(val, ".") == 0 || strcmp(val, "!") == 0 ||
            strcmp(val, "?") == 0 || strcmp(val, "...") == 0);
}

// тут определяем ставить ли пробел между словами
static bool needSpaceBetween(Token* curr, Token* next) {
    if (!curr || !next || next->type == TOK_EOF) return false;

    if (curr->type == TOK_PUNCTUATION && strcmp(curr->value, "(") == 0) return false;

    if (next->type == TOK_PUNCTUATION && strcmp(next->value, ")") == 0) return false;

    // перед точками и запятыми пробел не нужен
    if (next->type == TOK_PUNCTUATION) {
        char c = next->value[0];
        if (c == '.' || c == ',' || c == ':' || c == ';' || c == '!' || c == '?') return false;
    }

    // а вот вокруг тире пробелы нужны
    if (curr->type == TOK_PUNCTUATION && strcmp(curr->value, "-") == 0) return true;
    if (next->type == TOK_PUNCTUATION && strcmp(next->value, "-") == 0) return true;

    if (next->type == TOK_PUNCTUATION && strcmp(next->value, "(") == 0) return true;

    return true;
}

// запись результата в файл с нумерацией
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

    Token *curr = head;
    while (curr && curr->type != TOK_EOF) {

        // пишем номер предложения в начале
        if (startOfSentence) {
            string numStr = to_string(sentenceNumber) + ") ";
            out.write(numStr.c_str(), numStr.size());
            startOfSentence = false;
        }

        // если слово помечено, ставим скобки
        if (curr->type == TOK_IDENT) {
            if (curr->flag) out.write("[", 2);
            out.write(curr->value, strlen(curr->value));
            if (curr->flag) out.write("]", 2);
        } else {
            out.write(curr->value, strlen(curr->value));
        }

        if (needSpaceBetween(curr, curr->next)) {
            out.put(' ');
        }

        // проверяем конец предложения для счетчика
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

struct DefInfo {
    int count;
    vector<int> sentences;
    DefInfo() : count(0) {}
};

// формируем отчет сколько чего нашли
string buildDefinitionsReport(Token *head, int defCount) {
    map<string, DefInfo> stats;
    int sentenceIndex = 1;

    for (Token *cur = head; cur != NULL; cur = cur->next) {
        if (cur->type == TOK_IDENT && cur->flag && cur->value != NULL) {
            string word(cur->value);

            DefInfo &info = stats[word];
            info.count++;

            // добавляем номер предложения если его нет
            if (info.sentences.empty() || info.sentences.back() != sentenceIndex) {
                info.sentences.push_back(sentenceIndex);
            }
        }

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

    // выводим список слов и статистику
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
