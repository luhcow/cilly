#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define keywords_MAX  33
#define operators_MAX  41
//feature 1. 内存管理 2. 错误处理 3. 行列号

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
    char* lex;
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
    token* next;
}token;//每个分析完成的二元组

typedef struct Tokenlist
{
    token* next;
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
    char* y = "jjjj";
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
            text[i] = ' ';
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
            char temp[4];
            strcpy(temp, L->tail->lex);
            strcat(temp, tk->lex);
            if ((tempsyn = find_operators(temp)) > 0)
            {
                syn = tempsyn;
                tk->now = sizeof(temp) - 1;
                strcpy(L->tail->lex, temp);
                L->tail->end = tk->end;
                L->tail->syn = syn;
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
    L->tail = L->tail->next;
    L->tail->syn = syn;
    strcpy(L->tail->lex, tk->lex);
    L->tail->next = NULL;
    L->tail->begin = tk->begin;
    L->tail->end = tk->end;

    return 0;

}
void analyzer(char* text, char* argv[])
{
    //初始化token和token链
    TokenList* tokenlist = (TokenList*)malloc(sizeof(TokenList));
    tokenlist->now = 0;
    tokenlist->next = NULL;
    tokenlist->tail = tokenlist;

    struct Lex lex;
    lex.lex = (char*)malloc(127 * sizeof(char));
    lex.max = 30;
    lex.now = 0;
    lex.type = TOKEN_;
    lex.lex[0] = '\0';
    lex.begin.line = 0;
    lex.begin.con = 0;
    lex.end.line = 0;
    lex.end.con = 0;

    for (int i = 0, expression = 0; text[i] != -1 && expression != 9; )
    {
        // 0 first word 1 word 2 num 3 token结束时为空格 4 各类符号  8 异常处理
        //5 token结束时不是空白 6 ""''字符串
        //预期不出现除 \n \t之外的空白符制表符，全部按照异常处理
        char t = text[i];

        switch (expression)
        {
        case 0:
            if (!isprint(t) && t != '\n' && t != '\t')//非打印字符
                expression = 8;
            else if (isalpha(t) || t == '_')//保留字和标识符
                expression = 1;
            else if (isdigit(t))//常数
                expression = 2;
            else if (isspace(t))//空白，制表符空白符已按照异常处理
                i++;//第一个就是space跳过就行了
            else//各类符号
                expression = 4;
            break;
        case 1://保留字和标识符
            if (isalnum(t) || t == '_')
            {
                lex_push_back(&lex, t);
                expression = 1;
                i++;
            }
            else if (t == ' ')
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
            }
            else if (t == ' ')
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
        case 3://token结束时为空格，token正常结束
            tokenlist_pushback(&tokenlist, &lex);
            printf("get %s\t", lex.lex);
            lex.now = 0;
            lex.lex[0] = '\0';
            i++;
            expression = 0;

            break;
        case 4://!"#$%&'()*+,-./  :;<=>?@ [\]^_` {|}~ 直接pushback(),不是字符串就递交给token函数处理
            lex_push_back(&lex, t);
            if (t == '\"' || t == '\'')
            {
                expression = 6;
                i++;
                break;
            }
            else
            {
                lex.type = TOKEN_OPERATOR;
                tokenlist_pushback(&tokenlist, &lex);
                printf("get %s\t", lex.lex);
                lex.now = 0;
                lex.lex[0] = '\0';
                i++;
                expression = 0;
            }
            break;
        case 5://这个字符送进first word 重新处理
            tokenlist_pushback(&tokenlist, &lex);
            printf("get %s\t", lex.lex);
            lex.now = 0;
            lex.lex[0] = '\0';
            lex.type = TOKEN_;
            expression = 0;

            break;
        case 6://字符串处理，第一个引号已经在 case 4 处理		
            lex_push_back(&lex, t);
            if (t == '\"')//字符串结束，这个 text 已经 pushback 了
            {
                lex.type = TOKEN_STRING;
                expression = 3;
                break;
            }
            else if (t == '\'')
            {
                lex.type = TOKEN_CHAR;
                expression = 3;
                break;
            }
            else
            {
                i++;
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
                TokenList* temp = tokenlist;
                while (temp->next != NULL)
                {
                    fprintf(out, "{%s, %d,(begin lin: %d con: %d , end lin: %d con: %d)}\t", temp->next->lex, temp->next->syn, temp->next->begin.line, temp->next->begin.con, temp->next->end.line, temp->next->end.con);
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
                printf("非预期的字符");
                exit(1);
            }
        default:
            break;
        }

    }
}



typedef enum
{
    // 关键字
    DATA_SIGNED, DATA_UNSIGNED,
    DATA_CHAR, DATA_SHORT, DATA_INT, DATA_LONG,
    DATA_FLOAT, DATA_DOUBLE,
    DATA_STRUCT, DATA_UNION, DATA_ENUM, DATA_VOID,
    DATA_IF, DATA_ELSE, DATA_SWITCH, DATA_CASE, DATA_DEFAULT,
    DATA_WHILE, DATA_DO, DATA_FOR,
    DATA_BREAK, DATA_CONTINUE, DATA_RETURN, DATA_GOTO,
    DATA_CONST, DATA_SIZEOF, DATA_TYPEDEF,
} DataType;


typedef union
{
    int _int;
}variable_value;

typedef  struct
{
    TokenType type;
    DataType data_type;
    char* name;
    variable_value value;
}Token;

int main(int argc, char* argv[])
{
    char* text = (char*)malloc(100000 * sizeof(char));
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
        putchar(ch);
        text[count] = ch;
        count++;
    }
    text[count] = '\0';
    fclose(fp);
    printf("File %s has %lu characters\n", argv[1], count);


    clear_comments(text, count);
    printf("\n已经行删除\n");
    for (int l = 0; text[l] != '\0'; l++)
    {
        putchar(text[l]);
    }
    printf("\n已经行删除\n");

    analyzer(text, argv);

    return 0;
}