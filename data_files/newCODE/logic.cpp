#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fstream>      // Для работы с файловым вводом/выводом
#include <string>       // Для использования std::string
#include <commdlg.h>    // Для диалога открытия файла (WinAPI)
using namespace std;

// Глобальные переменные для лексера (входная строка и текущая позиция)
static const char *src;
static int pos = 0;

// Массив окончаний (суффиксов) для определения "определений" (слова, считающиеся определениями)
static char a[371][4] = {
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
    "скл", "ежо", "жёл", "уим", "ыро", "ягл", "ёдр", "яро"
};

// Перечисление типов токенов (структура осталась прежней)
typedef enum {
    TOK_EOF,         // Конец входного потока
    TOK_IDENT,       // Идентификатор (слово)
    TOK_PUNCTUATION, // Знаки препинания (и прочие отдельные символы)
    TOK_SPACE        // Пробельные символы (не используется в исходном анализе)
} TokenT;

typedef struct Token {
    TokenT type;
    bool flag;       // Флаг пометки "определения" (true, если слово оканчивается одним из указанных суффиксов)
    char *value;
    struct Token *prev;
    struct Token *next;
} Token;

// Функция для создания нового токена (выделяет память под структуру и копирует значение)
Token *create_token(TokenT type, const char *value) {
    Token *token = (Token*)malloc(sizeof(Token));
    token->type = type;
    token->value = strdup(value);  // дублируем строковое значение токена
    token->prev = NULL;
    token->next = NULL;
    token->flag = false;
    return token;
}

// Функция для добавления токена в конец двусвязного списка токенов
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

// Функция для освобождения памяти всего списка токенов
void free_tokens(Token *head) {
    Token *current = head;
    while (current != NULL) {
        Token *next = current->next;
        free(current->value);  // освобождаем строку значения токена
        free(current);         // освобождаем сам элемент Token
        current = next;
    }
}

// Функция для проверки, является ли символ русской буквой (кодировка Windows-1251)
int is_russian_letter(int ch) {
    // Русские буквы в Windows-1251: от 0xC0 до 0xFF, а также 'Ё' (0xA8) и 'ё' (0xB8)
    return (ch >= 0xC0 && ch <= 0xFF) || ch == 0xA8 || ch == 0xB8;
}

// Функция для проверки символа на принадлежность к знакам препинания
int is_punctuation(int ch) {
    // Рассматриваем распространенные знаки препинания и некоторые спецсимволы
    return ch == ',' || ch == '.' || ch == '!' || ch == '?' ||
           ch == ';' || ch == ':' || ch == '-' || ch == '(' ||
           ch == ')' || ch == '\"' || ch == '\'' || ch == '…';
}

// Функция для пропуска пробельных символов в исходном тексте
void skip_whitespace() {
    // Пропускаем все символы, для которых isspace() возвращает true (пробелы, табуляции, переводы строки и т.д.)
    while (src[pos] != '\0' && isspace((unsigned char)src[pos])) {
        pos++;
    }
}

// Функция для чтения идентификатора (слова) из входной строки.
// Считывает непрерывную последовательность символов, подходящих под критерий "буква/подчеркивание/дефис".
char *read_identifier() {
    int start = pos;
    // Цикл продолжается, пока текущий символ является буквой (русской или латинской), символом '_' или '-'
    while (src[pos] != '\0' && (is_russian_letter((unsigned char)src[pos]) ||
                                isalpha((unsigned char)src[pos]) ||
                                src[pos] == '_' ||
                                src[pos] == '-')) {
        pos++;
    }
    int length = pos - start;
    if (length == 0) return NULL;  // на случай, если по каким-то причинам длина 0 (не должно случаться здесь)
    // Выделяем новую строку для идентификатора и копируем в неё соответствующую подстроку из исходного текста
    char *ident = (char*)malloc(length + 1);
    strncpy(ident, &src[start], length);
    ident[length] = '\0';  // добавляем завершающий нуль-символ
    return ident;
}

// Функция для чтения знака препинания или отдельного символа из входной строки.
char *read_punctuation() {
    int start = pos;
    // Проверяем специальный случай: многоточие "..."
    if (src[pos] == '.' && src[pos + 1] == '.' && src[pos + 2] == '.') {
        pos += 3;  // пропускаем три точки как единый символ
        char *punct = (char*)malloc(4);
        strncpy(punct, &src[start], 3);
        punct[3] = '\0';
        return punct;
    }
    // Проверяем одиночные знаки препинания из списка is_punctuation
    if (is_punctuation((unsigned char)src[pos])) {
        pos++;
        char *punct = (char*)malloc(2);
        punct[0] = src[start];
        punct[1] = '\0';
        return punct;
    }
    return NULL;  // если текущий символ не является знаками...писка (не должно произойти, вызовется для других символов в lex)
}

// Основная функция лексического анализа: разбивает входную строку на токены.
Token *lex(const char *input) {
    src = input;   // устанавливаем глобальный указатель на начало входной строки
    pos = 0;       // начинаем с начала (позиция 0)

    Token *head = NULL;
    Token *tail = NULL;
    // Проходим по всей строке символ за символом
    while (src[pos] != '\0') {
        // Пропускаем пробельные символы между значимыми токенами
        skip_whitespace();
        if (src[pos] == '\0') break;  // если достигнут конец строки после пропуска пробелов, выходим

        char current = src[pos];
        // Если символ обозначает начало слова (русская или английская буква)
        if (is_russian_letter((unsigned char)current) || isalpha((unsigned char)current)) {
            // Читаем полный идентификатор (слово) начиная с этой позиции
            char *ident = read_identifier();
            if (ident != NULL) {
                Token *token = create_token(TOK_IDENT, ident);
                add_token(&head, &tail, token);
                free(ident);  // освобождаем временный буфер, strdup уже скопировал в token->value
            }
            continue;  // переходим к следующему фрагменту входной строки
        }
        // Иначе, если символ является знаком препинания
        else if (is_punctuation((unsigned char)current)) {
            // Читаем все, что относится к этому знаку препинания (учитывая многоточие)
            char *punct = read_punctuation();
            if (punct != NULL) {
                Token *token = create_token(TOK_PUNCTUATION, punct);
                add_token(&head, &tail, token);
                free(punct);
            }
            continue;  // продолжаем цикл
        }
        // Любой другой символ (цифры, специальные символы и... т.д.), не отнесённый к буквам или стандартным знакам препинания
        else {
            // Создаём токен для этого одиночного символа, трактуя его как пунктуацию/символ
            char symbol[2] = { current, '\0' };
            Token *token = create_token(TOK_PUNCTUATION, symbol);
            add_token(&head, &tail, token);
            pos++;  // переходим к следующему символу исходного текста
        }
    }
    // В конец списка токенов добавляем специальный токен конца файла (EOF)
    Token *eof = create_token(TOK_EOF, "EOF");
    add_token(&head, &tail, eof);
    return head;
}

// Функция для пометки токенов-слов, оканчивающихся на заданные суффиксы (из массива a).
void mark_tokens_with_endings(Token *head) {
    Token *current = head;
    while (current != NULL) {
        // Проверяем только токены типа TOK_IDENT (только слова имеют смысл проверять)
        if (current->type == TOK_IDENT && current->value != NULL) {
            size_t len = strlen(current->value);
            if (len >= 3) {
                // Извлекаем последние 3 символа слова
                char last_three[4];
                strncpy(last_three, current->value + len - 3, 3);
                last_three[3] = '\0';  // добавляем завершающий нуль
                // Проверяем, присутствует ли такая последовательность в списке окончаний
                bool found = false;
                for (int i = 0; i < 371; i++) {
                    if (strcmp(last_three, a[i]) == 0) {
                        found = true;
                        break;
                    }
                }
                // Устанавливаем флаг токена: true, если совпадение найдено, иначе false
                current->flag = found;
            } else {
                // Если слово короче 3 символов, оно не может содержать указанный суффикс
                current->flag = false;
            }
        }
        current = current->next;
    }
}

// Новая функция: чтение текста из файла (кодировка Windows-1251) в строку.
// Возвращает true при успешном чтении, false если файл не удалось открыть.
// Считанный текст (включая переводы строк) помещается в outContent.
bool readTextFromFile(const string &filename, string &outContent) {
    // Открываем файл в двоичном режиме, чтобы сохранить все байты без изменений (особенно переводы строк \r\n)
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        // Если файл не открылся, возвращаем false (ошибка открытия)
        return false;
    }
    // Читаем весь файл в строку outContent
    // Перемещаемся в конец, считываем размер, затем выделяем соответствующий буфер
    file.seekg(0, ios::end);
    streamoff fileSize = file.tellg();
    if (fileSize < 0) fileSize = 0;
    outContent.resize((size_t)fileSize);
    file.seekg(0, ios::beg);
    // Читаем все байты файла в строку
    file.read(&outContent[0], fileSize);
    file.close();
    // Теперь outContent содержит полный текст файла (в кодировке Windows-1251), включая перевод строки, если он был.
    return true;
}

// Новая функция: обратная сборка текста из списка токенов с добавлением специальных меток перед "определениями".
// Принимает на вход указатель на начало списка токенов (head) и исходный текст (originalText).
// Возвращает строку, представляющую исходный текст, где перед каждым словом с флагом flag=true добавлен символ '±'.
string assembleTextFromTokens(Token *head, const string &originalText) {
    string result;
    result.reserve(originalText.size() + 100); // резервируе...м память с запасом (примерно), чтобы улучшить производительность
    Token *currentToken = head;
    string currentWord = "";  // накопитель для текущего слова при обходе исходного текста

    // Проходим по каждому символу исходного текста
    for (size_t i = 0; i < originalText.size(); ++i) {
        unsigned char c = (unsigned char) originalText[i];
        // Проверяем, является ли символ частью слова (буква латинская или русская, либо '_' или '-')
        if (is_russian_letter(c) || isalpha(c) || c == '_' || c == '-') {
            // Если символ буква/подчёркивание/дефис, добавляем его к текущему слову
            currentWord.push_back((char)c);
        } else {
            // Если встретился символ, не принадлежащий слову (пробел, знак препинания или другой символ)
            if (!currentWord.empty()) {
                // Если накоплено текущее слово, значит слово завершилось перед текущим символом.
                // Найдём соответствующий токен-слово в списке токенов (он должен идти первым среди следующих токенов).
                while (currentToken != NULL && currentToken->type != TOK_IDENT) {
                    currentToken = currentToken->next;
                }
                // Если найден токен-слово и он помечен как "определение", добавляем специальный символ перед словом
                if (currentToken != NULL && currentToken->type == TOK_IDENT) {
                    if (currentToken->flag) {
                        result.push_back('±');  // добавляем маркер "определение" перед словом
                    }
                    result += currentToken->value;  // добавляем само слово в результат
                    currentToken = currentToken->next; // переходим к следующему токену в списке
                } else {
                    // На случай, если токен не найден (не должно происходить при корректной работе лексера)
                    result += currentWord;
                }
                currentWord.clear(); // очищаем накопитель слова
            }
            // Теперь обрабатываем текущий не буквенный символ:
            result.push_back((char)c);
            // Символ c сам по себе не часть слова, поэтому просто добавляем его в результат как есть 
            // (это может быть пробел, знак препинания, перевод строки и т.п., они сохраняются без изменений)
        }
    }
    // После окончания цикла возможно осталось накопленное слово (если текст заканчивался буквой).
    if (!currentWord.empty()) {
        // Обрабатываем оставшееся слово аналогично: ищем следующий токен-слово
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
    // На этом этапе строка result содержит исходный текст, где перед всеми требуемыми словами вставлен символ '±'.
    return result;
}

// Новая функция: запись обработанного текста (с пометками) в отдельный файл.
// Принимает имя выходного файла (outputFilename) и строку текста (text) для записи.
// Возвращает true, если запись прошла успешно; false, если возникла ошибка при открытии/записи файла.
bool writeProcessedTextToFile(const string &outputFilename, const string &text) {
    // Открываем файл для записи в двоичном режиме, чтобы ко...д содержимым был полным (никаких преобразований строк окончания)
    ofstream outFile(outputFilename, ios::binary);
    if (!outFile.is_open()) {
        // Не удалось открыть файл для записи (например, нет прав или неверное имя)
        return false;
    }
    // Записываем всю строку текста в файл. Поскольку мы открыли файл в двоичном режиме, 
    // все символы (в том числе русские буквы и символ '±') будут записаны "как есть" в кодировке Windows-1251.
    outFile.write(text.c_str(), text.size());
    outFile.close();
    return true;
}

// Новая функция: Запуск простейшего графического интерфейса (WinAPI) для выбора файла и запуска обработки.
// Вызывается, если программа запущена с параметром -gui. 
// Открывает стандартный диалог выбора файла, затем вызывает функции чтения, обработки и записи, 
// и отображает сообщение о результате.
void runGUI() {
    // Структура для параметров диалога открытия файла
    OPENFILENAME ofn;
    char fileName[MAX_PATH] = "";            // буфер для пути к выбранному файлу
    ZeroMemory(&ofn, sizeof(ofn));           // обнуляем структуру
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;                    // у диалога нет владельца (можно привязать к окну, если оно было)
    // Фильтр файлов: показываем только текстовые файлы и все файлы
    ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;  // файл должен существовать
    ofn.lpstrTitle = "Выберите текстовый файл для обработки";
    // Открываем модальный диалог "Открыть файл"
    if (!GetOpenFileName(&ofn)) {
        // Если пользователь отменил выбор файла или произошла ошибка
        MessageBox(NULL, "Файл не выбран. Работа приложения завершена.", "Информация", MB_OK | MB_ICONINFORMATION);
        return;
    }
    // Пользователь выбрал файл, путь сохранён в fileName
    string inputPath = fileName;
    string content;
    // Читаем файл
    if (!readTextFromFile(inputPath, content)) {
        // Если чтение не удалось (например, нет доступа или файл не текстовый), сообщаем об ошибке
        MessageBox(NULL, "Не удалось открыть выбранный файл.", "Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    // Выполняем лексический анализ и пометку токенов
    Token *tokens = lex(content.c_str());
    mark_tokens_with_endings(tokens);
    // Обратно собираем текст с отметками "определений"
    string outputText = assembleTextFromTokens(tokens, content);
    // Формируем имя выходного файла. Например, для "input.txt" сделаем "input_marked.txt"
    string outputPath;
    // Найдём имя файла без пути
    size_t pos = inputPath.find_last_of("\\/");
    string fileNameOnly = (pos == string::npos) ? inputPath : inputPath.substr(pos + 1);
    // Убираем расширение .txt (если есть) и добавляем суффикс "_marked"
    size_t dotPos = fileNameOnly.find_last_of('.');
    if (dotPos != string::npos) {
        // Есть точка, отделяющая расширение
        string baseName = fileNameOnly.substr(0, dotPos);
        string extension = fileNameOnly.substr(dotPos);  // включая точку
        outputPath = baseName + "_marked" + extension;
    } else {
        // Если расширения нет, просто добавляем суффикс
        outputPath = fileNameOnly + "_marked.txt";
    }
    // Пишем результат в выходной файл
    if (!writeProcessedTextToFile(outputPath, outputText)) {
        // Если не удалось записать файл
        MessageBox(NULL, "Не удалось создать файл для записи результата.", "Ошибка", MB_OK | MB_ICONERROR);
        free_tokens(tokens);
        return;
    }
    // Освобождаем память, занятую списком токенов
    free_tokens(tokens);
    // Уведомляем пользователя об успешном завершении
    string message = "Обработанный текст сохранен в файл:\n" + outputPath;
    MessageBox(NULL, message.c_str(), "Готово", MB_OK | MB_ICONINFORMATION);
}
