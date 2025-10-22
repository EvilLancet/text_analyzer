#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

using namespace std;

static const char *src;
static int pos = 0;

char a[371][4] = {"йся", "мся", "его", "шей", "ого", "еся", "шим", "ших", "ему", "ной", "ося",
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
              "ыты", "ято", "яты", "сый", "зых", "зым", "тён", "шан", "ето", "сён", "жио",
              "озо", "кко", "пен", "пна", "пны", "пно", "еян", "аян", "аво", "ёрт", "нан",
              "рыт", "зые", "зое", "зый", "пым", "пых", "жьи", "уча", "бый", "оён", "утл",
              "ьен", "щна", "щны", "щно", "гол", "ёво", "шнл", "роч", "общ", "реч", "исп",
              "нгл", "пые", "ьфо", "ндж", "ьче", "лёш", "беж", "куб", "оей", "еол", "сск",
              "доп", "пое", "уфф", "тыш", "паш", "гро", "гос", "дум", "фри", "гри", "иже",
              "раб", "кит", "бён", "зён", "мыт", "аен", "пый", "оих", "яво", "ява", "явы",
              "еих", "пую", "еий", "еие", "еее", "пуч", "ыно", "вся", "усо", "хож", "щан",
              "ьин", "яст", "сих", "тот", "ъят", "гуч", "ёро", "еюю", "еяя", "осл", "вуч",
              "шея", "аён", "сёв", "воё", "гож", "узо", "куч", "оею", "уен", "оус", "зуч",
              "сех", "щов", "моё", "узд", "южи", "мих", "южа", "пуз", "здо", "юже", "упо",
              "ысо", "нюч", "муч", "тёв", "люж", "дуч", "всё", "ряч", "езв", "згл", "тящ",
              "ошл", "гок", "гки", "мои", "кны", "нущ", "дёв", "шлы", "ипл", "зва", "зво",
              "это", "егл", "тыж", "етх", "язо", "яхл", "дюж", "бущ", "тхл", "ядл", "вущ",
              "аов", "ёло", "ижн", "дащ", "лыс", "окр", "едл", "хуч", "ёгл", "ыхл", "ябо",
              "лощ", "уён", "ёзо", "тощ", "суч", "сяя", "сюю", "юно", "цно", "рюч", "рзо",
              "вещ", "орл", "ячо", "ябл", "упл", "одл", "туч", "юто", "моя", "агл", "хоч",
              "скл", "ежо", "жёл", "уим", "ыро", "ягл", "ёдр", "яро"};


typedef enum {
    TOK_EOF,        // Конец входного потока
    TOK_IDENT,      // Идентификатор (слово)
    TOK_PUNCTUATION,// Знаки препинания
    TOK_SPACE       // Пробельные символы (если нужно сохранять)
} TokenT;

typedef struct Token {
    TokenT type;
    bool flag;
    char *value;
    struct Token *prev;
    struct Token *next;
} Token;

// Функция для создания нового токена
Token *create_token(TokenT type, const char *value) {
    Token *token = (Token*)malloc(sizeof(Token));
    token->type = type;
    token->value = strdup(value);
    token->prev = NULL;
    token->next = NULL;
    token->flag = false;
    return token;
}

// Функция для добавления токена в конец списка
void add_token(Token **head, Token **tail, Token *token) {
    if (*head == NULL) {
        *head = token;
        *tail = token;
    } else {
        (*tail)->next = token;
        token->prev = *tail;
        *tail = token;
    }
}

// Функция для освобождения памяти списка токенов
void free_tokens(Token *head) {
    Token *current = head;
    while (current != NULL) {
        Token *next = current->next;
        free(current->value);
        free(current);
        current = next;
    }
}

// Функция для проверки русских символов (расширенная версия)
int is_russian_letter(int ch) {
    // Русские буквы в кодировке Windows-1251
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// Функция для проверки знаков препинания
int is_punctuation(int ch) {
    return ch == ',' || ch == '.' || ch == '!' || ch == '?' ||
           ch == ';' || ch == ':' || ch == '-' || ch == '(' ||
           ch == ')' || ch == '\"' || ch == '\'' || ch == '…';
}

// Функция для пропуска пробельных символов
void skip_whitespace() {
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) {
        pos++;
    }
}

// Функция для чтения идентификатора (слова)
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

// Функция для чтения знаков препинания
char *read_punctuation() {
    int start = pos;

    // Обработка многоточия
    if (src[pos] == '.' && src[pos + 1] == '.' && src[pos + 2] == '.') {
        pos += 3;
        char *punct = (char*)malloc(4);
        strncpy(punct, &src[start], 3);
        punct[3] = '\0';
        return punct;
    }

    // Одиночные знаки препинания
    if (is_punctuation(src[pos])) {
        pos++;
        char *punct = (char*)malloc(2);
        punct[0] = src[start];
        punct[1] = '\0';
        return punct;
    }

    return NULL;
}

// Основная функция лексического анализа
Token *lex(const char *input) {
    src = input;
    pos = 0;

    Token *head = NULL;
    Token *tail = NULL;

    while (src[pos] != '\0') {
        skip_whitespace();

        if (src[pos] == '\0') break;

        char current = src[pos];

        // Слова (русские и английские)
        if (is_russian_letter((unsigned char)current) || isalpha((unsigned char)current)) {
            char *ident = read_identifier();
            if (ident != NULL) {
                Token *token = create_token(TOK_IDENT, ident);
                add_token(&head, &tail, token);
                free(ident);
            }
            continue;
        }
        // Знаки препинания
        else if (is_punctuation(current)) {
            char *punct = read_punctuation();
            if (punct != NULL) {
                Token *token = create_token(TOK_PUNCTUATION, punct);
                add_token(&head, &tail, token);
                free(punct);
            }
            continue;
        }
        // Любые другие символы (цифры, специальные символы и т.д.)
        else {
            // Создаем токен для одиночного символа
            char symbol[2] = {current, '\0'};
            Token *token = create_token(TOK_PUNCTUATION, symbol);
            add_token(&head, &tail, token);
            pos++;
        }
    }

    // Добавляем токен конца файла
    Token *eof = create_token(TOK_EOF, "EOF");
    add_token(&head, &tail, eof);

    return head;
}

void print_tokens(Token *head) {
    const char *type_names[] = {
        "EOF", "IDENT", "PUNCTUATION", "SPACE"
    };

    Token *current = head;
    printf("Токены:\n");
    while (current != NULL) {
        // Для печати не-ASCII символов корректно
        if (current->type == TOK_IDENT) {
            if(current->flag == true) printf("Опр!");
            printf("  [%s: ", type_names[current->type]);
            // Печатаем русский текст напрямую
            printf("%s", current->value);
            printf("]\n");
        } else {
            printf("  [%s: %s]\n", type_names[current->type], current->value);
        }
        current = current->next;
    }
}
// Функция для проверки окончания
void mark_tokens_with_endings(Token *head) {
    Token *current = head;

    while (current != NULL) {
        // Проверяем только TOK_IDENT
        if (current->type == TOK_IDENT && current->value != NULL) {
            size_t len = strlen(current->value);

            // Проверяем, что строка достаточно длинная (минимум 3 символа)
            if (len >= 3) {
                // Извлекаем последние 3 символа
                char last_three[4];
                strncpy(last_three, current->value + len - 3, 3);
                last_three[3] = '\0'; // Завершающий нуль

                // Проверяем наличие в массиве
                bool found = false;
                for (int i = 0; i < 371; i++) {
                    if (strcmp(last_three, a[i]) == 0) {
                        found = true;
                        break;
                    }
                }

                current->flag = found;
            } else {
                // Если строка короче 3 символов, флаг = false
                current->flag = false;
            }
        }
        current = current->next;
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    const char *input = "Лингвистика — это научное исследование языка. Важнейшим аспектом этой дисциплины является морфология. Синтаксис занимается правилами построения осмысленных предложений. Фонетика исследует звуки речи. Семантика анализирует скрытые значения. Лингвисты часто проводят точные эксперименты. Современные исследования используют компьютерные технологии. Например, корпусная лингвистика стала очень популярной. Эти методы позволяют получать объективные данные.";
    printf("Входное выражение: %s\n", input);

    Token *tokens = lex(input);
    mark_tokens_with_endings(tokens);
    print_tokens(tokens);
    free_tokens(tokens);

    return 0;
}
