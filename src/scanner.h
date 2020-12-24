#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================
struct Scanner
{
    const char* start;
    const char* current;
    i32 line;
};
Scanner scanner;

enum TokenType
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR,
    TOKEN_EOF
};

struct Token
{
    TokenType type;
    const char* start;
    i32 length;
    i32 line;
};
// =================================================================

// =================================================================
// API Functions
// =================================================================
void init_scanner(const char* source);
Token scan_token();
// =================================================================


// =================================================================
// Internal Functions
// =================================================================
static bool is_alpha(char c);
static bool is_digit(char c);
static bool is_at_end();
static char advance();
static char peek();
static char peek_next();
static bool match(char expected);
static Token make_token(TokenType type);
static Token error_token(const char* message);
static void skip_whitespace();
static TokenType check_keyword(i32 start, i32 length, const char* rest, TokenType type);
static TokenType identifier_type();
static Token identifier();
static Token number();
static Token string();
// =================================================================



#endif
