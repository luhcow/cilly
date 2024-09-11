//feature  1. 使用安全函数 2. 不同进制的数字字面量 3. 更好的识别非法字符 4. 真正的错误处理 5. 整理代码
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define keywords_MAX  33
#define operators_MAX  41

const char* keywords[keywords_MAX] = {
    // 字面值: 标识符, 字符, 字符串, 数字
    "identifier", "character", "string", "number",
    // 关键字
    "signed", "unsigned",
    "char", "short", "int", "long",
    "float", "double",
    "struct", "union", "enum", "void",
    "if", "else", "switch", "case", "default",
    "while", "do", "for",
    "break", "continue", "return", "goto",
    "const", "sizeof", "typedef",
    // 辅助Token
    "error", "eof"
};


const char* operators[operators_MAX] = {
    // single-character tokens
    "(", ")", "[", "]",
    "{", "}",
    ",", ".", ";",
    "~",
    // one or two character tokens
    "+", "++", "+=",
    "-", "--", "-=", "->",
    "*", "*=",
    "/", "/=",
    "%", "%=",
    "&", "&=", "&&",
    "|", "|=", "||",
    "^", "^=",
    "=", "==",
    "!", "!=",
    "< ", " <= ", " << ",
    ">", ">=", ">>",
};

typedef enum
{
    // single-character tokens
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,		// '(', ')'
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,	// '[', ']'
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,  		// '{', '}'
    TOKEN_COMMA, TOKEN_DOT, TOKEN_SEMICOLON,	// ',', '.', ';'
    TOKEN_TILDE,  // '~'
    // one or two character tokens
    TOKEN_PLUS, TOKEN_PLUS_PLUS, TOKEN_PLUS_EQUAL, // '+', '++', '+='
    // '-', '--', '-=', '->'
    TOKEN_MINUS, TOKEN_MINUS_MINUS, TOKEN_MINUS_EQUAL, TOKEN_MINUS_GREATER,
    TOKEN_STAR, TOKEN_STAR_EQUAL,				// '*', '*='
    TOKEN_SLASH, TOKEN_SLASH_EQUAL, 		// '/', '/=', 
    TOKEN_PERCENT, TOKEN_PERCENT_EQUAL, // '%', '%='
    TOKEN_AMPER, TOKEN_AMPER_EQUAL, TOKEN_AMPER_AMPER, // '&', '&=', '&&'
    TOKEN_PIPE, TOKEN_PIPE_EQUAL, TOKEN_PIPE_PIPE,	// '|', '|=', '||'
    TOKEN_HAT, TOKEN_HAT_EQUAL, // '^', '^='
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, // '=', '=='
    TOKEN_BANG, TOKEN_BANG_EQUAL,	  // '!', '!='
    TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_LESS_LESS, 	// '<', '<=', '<<'
    TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_GREATER_GREATER, // '>', '>=', '>>'
    // 字面值: 标识符, 字符, 字符串, 数字
    TOKEN_IDENTIFIER, TOKEN_CHARACTER, TOKEN_STRING, TOKEN_NUMBER,
    // 关键字
    TOKEN_SIGNED, TOKEN_UNSIGNED,
    TOKEN_CHAR, TOKEN_SHORT, TOKEN_INT, TOKEN_LONG,
    TOKEN_FLOAT, TOKEN_DOUBLE,
    TOKEN_STRUCT, TOKEN_UNION, TOKEN_ENUM, TOKEN_VOID,
    TOKEN_IF, TOKEN_ELSE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,
    TOKEN_WHILE, TOKEN_DO, TOKEN_FOR,
    TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_RETURN, TOKEN_GOTO,
    TOKEN_CONST, TOKEN_SIZEOF, TOKEN_TYPEDEF,
    // 辅助Token
    TOKEN_ERROR, TOKEN_EOF, TOKEN_, TOKEN_KEYWORD, TOKEN_OPERATOR
} TokenType;

struct line_con
{
    unsigned int line;
    unsigned int con;
};

struct Lex
{
    char lex[255];
    int now;
    int max;
    TokenType type;// 字面值: 标识符, 字符, 字符串, 数字
    struct line_con begin;
    struct line_con end;
};//用来储存未完成和已完成的词

typedef struct token_node
{
    char lex[255];
    int syn;
    struct line_con begin;
    struct line_con end;
    struct token_node* next;
}token;//每个分析完成的二元组

typedef struct Tokenlist
{
    token* head;
    token* tail;
    int now;
}TokenList;//二元组的数组

int find_operators(char word[])
{
    for (int i = 0; i < operators_MAX; i++)
    {
        if (strcmp(word, operators[i]) == 0)
            return i;
    }
    return 0;
}

int find_keyWords(char word[])
{
    for (int i = 0; i < keywords_MAX; i++)
    {
        if (strcmp(word, keywords[i]) == 0)
            return i + TOKEN_SIGNED;
    }
    return 0;
}

int lex_push_back(struct Lex* tk, char t)//为词的末尾添加字符，在main函数已预先初始化
{
    tk->lex[tk->now] = t;
    tk->now++;
    tk->lex[tk->now] = '\0';
    return 0;
}

int clear_comments(char text[], int count)
{
    int switch_on = 0;
    for (int i = 0; i < count && text[i] != '\0'; )
    {
        switch (switch_on)
        {
        case 0:
            if (text[i] == '/')
            {
                switch_on = 1;
            }
            i++;
            break;
        case 1:
            if (text[i] == '/')
            {
                switch_on = 2;
                text[i - 1] = ' ';
                text[i] = ' ';
            }
            else if (text[i] == '*')
            {
                switch_on = 3;
                text[i - 1] = ' ';
                text[i] = ' ';
            }
            else switch_on = 0;
            i++;
            break;
        case 2:
            if (text[i] == '\n')
            {
                switch_on = 0;
            }
            else
            {
                text[i] = ' ';
            }
            i++;
            break;
        case 3:
            if (text[i] == '*')
            {
                if (text[i + 1] == '/')
                {
                    switch_on = 0;
                    text[i] = ' ';
                    i = i + 1;
                }
            }
            if (text[i] != '\n')
            {
                text[i] = ' ';
            }
            i++;
            break;
        default:
            break;
        }
    }
    return 0;
}

int tokenlist_pushback(TokenList* L, struct Lex* tk)//这里还要处理每个 token 的 syn 以及双目运算符
{
    int syn, tempsyn;
    switch (tk->type)
    {
    case TOKEN_OPERATOR:
        syn = find_operators(tk->lex);
        if (L->now > 0)
        {
            char temp[255];
            strncpy(temp, L->tail->lex, sizeof(temp));
            strncat(temp, tk->lex, sizeof(temp));
            if ((tempsyn = find_operators(temp)) > 0)
            {
                syn = tempsyn;
                tk->now = sizeof(temp) - 1;
                strncpy(L->tail->lex, temp, sizeof(temp));
                L->tail->end = tk->end;
                L->tail->syn = syn;
                L->now++;
                return 0;
            }
        }
        break;
    case TOKEN_KEYWORD:
        if (syn = find_keyWords(tk->lex))
            ;
        else
            syn = TOKEN_IDENTIFIER;
        break;
    case TOKEN_NUMBER: case TOKEN_CHAR: case TOKEN_STRING:
        syn = tk->type;
        break;

    default:
        break;
    }

    L->tail->next = (token*)malloc(sizeof(token));
    if (L->tail->next == NULL)
    {
        // 处理内存分配失败的情况
        // 这里可以根据实际情况进行处理，例如打印错误信息并退出程序

        exit(1);
    }
    L->tail = L->tail->next;
    L->tail->syn = syn;
    strcpy(L->tail->lex, tk->lex);
    L->tail->next = NULL;
    L->tail->begin = tk->begin;
    L->tail->end = tk->end;
    L->now++;

    return 0;

}
int analyzer(char* text, char* argv[])
{
    //初始化token和token链
    TokenList* tokenlist = (TokenList*)malloc(sizeof(TokenList));
    if (tokenlist == NULL)
    {
        // 处理内存分配失败的情况
        return -1;
    }
    tokenlist->now = 0;
    tokenlist->head = (token*)malloc(sizeof(token));
    if (tokenlist->head == NULL)
    {
        // 处理内存分配失败的情况
        free(tokenlist);
        return -1;
    }
    tokenlist->head->next = NULL;
    tokenlist->tail = tokenlist->head;

    struct Lex lex;
    lex.max = 30;
    lex.now = 0;
    lex.type = TOKEN_;
    lex.lex[0] = '\0';
    lex.begin.line = 0;
    lex.begin.con = 0;
    lex.end.line = 0;
    lex.end.con = 0;

    for (int i = 0, expression = 0, con = 1, line = 1; text[i] != -1 && expression != 9; )
    {
        // 0 first word 1 关键字和标识符 2 数值字面量 3 token 结束时为 space 4 各类符号  
        //5 token结束时不是 space 6 ""''字符和字符串字面量 8 异常处理和正常结束

        char t = text[i];

        switch (expression)
        {
        case 0:
            if (!isprint(t) && t != '\n' && t != '\t')//非打印字符
                expression = 8;
            else if (isalpha(t) || t == '_')//保留字和标识符
            {
                expression = 1;
                lex.begin.con = con;
                lex.begin.line = line;
            }
            else if (isdigit(t))//常数
            {
                expression = 2;
                lex.begin.con = con;
                lex.begin.line = line;
            }
            else if (isspace(t))//空白
            {
                i++;//第一个就是space跳过就行了
                con++;
                if (t == '\n')
                {
                    line++;
                    con = 1;
                }

            }

            else//各类符号
            {
                expression = 4;
                lex.begin.con = con;
                lex.begin.line = line;
            }
            break;
        case 1://保留字和标识符
            if (isalnum(t) || t == '_')
            {
                lex_push_back(&lex, t);
                expression = 1;
                i++;
                con++;
            }
            else if (isspace(t))
            {
                lex.type = TOKEN_KEYWORD;
                expression = 3;
            }
            else
            {
                lex.type = TOKEN_KEYWORD;
                expression = 5;
            }
            break;
        case 2://数字
            if (isalnum(t))
            {
                lex_push_back(&lex, t);
                expression = 2;
                i++;
                con++;
            }
            else if (isspace(t))
            {
                lex.type = TOKEN_NUMBER;
                expression = 3;
            }
            else
            {
                lex.type = TOKEN_NUMBER;
                expression = 5;
            }
            break;
        case 3://token正常结束
            lex.end.con = con;
            lex.end.line = line;
            tokenlist_pushback(tokenlist, &lex);
            lex.now = 0;
            lex.lex[0] = '\0';
            i++;
            con++;
            if (t == '\n')
            {
                con = 1;
                line++;
            }
            expression = 0;

            break;
        case 4://!"#$%&'()*+,-./  :;<=>?@ [\]^_` {|}~ 直接pushback(),不是字符串就递交给token函数处理
            lex_push_back(&lex, t);
            if (t == '\"' || t == '\'')
            {
                expression = 6;
                i++;
                con++;
                break;
            }
            else
            {
                lex.type = TOKEN_OPERATOR;
                lex.end.con = con + 1;
                lex.end.line = line;
                tokenlist_pushback(tokenlist, &lex);
                lex.now = 0;
                lex.lex[0] = '\0';
                i++;
                con++;
                expression = 0;
            }
            break;
        case 5://这个字符送进first word 重新处理
            lex.end.con = con;
            lex.end.line = line;
            tokenlist_pushback(tokenlist, &lex);
            lex.now = 0;
            lex.lex[0] = '\0';
            lex.type = TOKEN_;
            expression = 0;

            break;
        case 6://字符串处理，第一个引号已经在 case 4 处理		
            lex_push_back(&lex, t);
            if (t == '\"' && lex.lex[lex.now - 1] != '//')//字符串结束，这个 text 已经 pushback 了
            {
                lex.type = TOKEN_STRING;
                expression = 3;
                break;
            }
            else if (t == '\'' && lex.lex[lex.now - 1] != '//')
            {
                lex.type = TOKEN_CHAR;
                expression = 3;
                break;
            }
            else
            {
                i++;
                con++;
                expression = 6;
                break;
            }
        case 8:
            putchar(t);
            if (t == '\0')
            {
                char name[127];
                strncpy(name, argv[1], 122);
                name[122] = '\0';
                strcat(name, "lexes");
                FILE* out;
                if ((out = fopen(name, "w")) == NULL)
                {                       // 以写模式打开文件
                    fprintf(stderr, "Can't create output file.\n");
                    exit(3);
                }
                // 拷贝数据
                token* temp = tokenlist->head;
                while (temp->next != NULL)
                {
                    fprintf(out, "{%s, %d,(begin lin: %d con: %d , end lin: %d con: %d)}\n", temp->next->lex, temp->next->syn, temp->next->begin.line, temp->next->begin.con, temp->next->end.line, temp->next->end.con);
                    temp = temp->next;
                }
                fprintf(out, "\n");
                //putchar(EOF);
                // 收尾工作
                if (fclose(out) != 0)
                    fprintf(stderr, "Error in closing files\n");
                return 0;
            }
            else
            {
                printf("非预期的字符于 line: %d con: %d\n", lex.end.line, lex.end.con);
                exit(1);
            }
        default:
            break;
        }

    }
}

const int text_size = 1000000;

int main(int argc, char* argv[])
{
    char* text = (char*)malloc(text_size * sizeof(char));
    int ch;            // 读取文件时，存储每个字符的地方
    FILE* fp;        // “文件指针”
    unsigned long count = 0;
    if (argc != 2)
    {
        printf("Usage: %s filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if ((fp = fopen(argv[1], "r")) == NULL)
    {
        printf("Can't open %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    while ((ch = getc(fp)) != EOF)
    {
        text[count] = ch;
        count++;
        if (count == text_size)
        {
            char* p = (char*)realloc(text, (text_size + 1000000) * sizeof(char));
            if (p != NULL)
                text = p;
            else
            {
                printf("Error in realloc\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    text[count] = '\0';
    fclose(fp);
    printf("File %s has %lu characters\n", argv[1], count);

    clear_comments(text, count);

    analyzer(text, argv);

    return 0;
}