#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================
struct Parser
{
    Token current;
    Token previous;

    bool had_error;
    bool panic_mode;

    ObjectStore* store;
    Table* strings;
};

enum Precedence
{
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
};

using ParseFn = void(*)(Parser*, bool);
struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

struct Local
{
    Token name;
    i32 depth;
    bool immutable;
};

struct Compiler
{
    Local locals[UINT8_COUNT];
    i32 local_count;
    i32 scope_depth;
};

ParseRule rules[TOKEN_EOF];


// =================================================================
// API Functions
// =================================================================
bool compile(const char* source, Chunk* chunk, ObjectStore* output_store, Table* output_strings);
void init_parse_rules();
// =================================================================

// =================================================================
// Internal functions
// =================================================================
static void error_at(Parser* parser, Token* token, const char* message);
static void error_at(Parser* parser, Token* token, const char* message);
static void error(Parser* parser, const char* message);

static void advance(Parser* parser);
static void consume(Parser* parser, TokenType type, const char* message);

static Chunk* current_chunk();

static void emit_byte(Parser* pasrer, u8 byte);
static void emit_bytes(Parser* pasrer, u8 byte_1, u8 byte_2);
static void emit_return(Parser* parser);
static u8 make_constant(Parser* parser, Value value);
static u8 identifier_constant(Parser* parser, Token* name);
static void emit_constant(Parser* parser);
static void end_compiler(Parser* parser);
static void parse_precedence(Parser* parser, Precedence precedence);
static ParseRule* get_rule(TokenType type);
static void expression(Parser* parser);
static void declaration(Parser* parser);
static void statement(Parser* parser);

static i32 resolve_local(Parser* parser, Compiler* compiler, Token* name, bool* immutable);
// =================================================================

#endif
