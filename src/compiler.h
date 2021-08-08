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

    b32 had_error;
    b32 panic_mode;

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

using ParseFn = void(*)(GarbageCollector*, Parser*, b32);
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
    b32 is_captured;
    b32 immutable;
};

struct Upvalue
{
    u8 index;
    b32 is_local;
};

enum FunctionType
{
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_SCRIPT
};

struct Global
{
    ObjString* name;
    b32 immutable;
};

Global globals[UINT8_COUNT];
i32 global_count;

struct Compiler
{
    struct Compiler* enclosing; // @Note(Niels): Change this to not be a linked list for perf
    
    ObjFunction* function;
    FunctionType type;
    
    Local locals[UINT8_COUNT];
    i32 local_count;

    Upvalue upvalues[UINT8_COUNT];
    
    i32 scope_depth;
};

ParseRule rules[TOKEN_EOF];


// =================================================================
// API Functions
// =================================================================
ObjFunction* compile(GarbageCollector* gc, const char* source, ObjectStore* output_store, Table* output_strings);
void mark_compiler_roots(GarbageCollector* gc);
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

static void emit_byte(GarbageCollector* gc, Parser* pasrer, u8 byte);
static void emit_bytes(GarbageCollector* gc, Parser* pasrer, u8 byte_1, u8 byte_2);
static void emit_return(GarbageCollector* gc, Parser* parser);
static u8 make_constant(GarbageCollector* gc, Parser* parser, Value value);
static u8 identifier_constant(GarbageCollector* gc, Parser* parser, Token* name);
static void emit_constant(GarbageCollector* gc, Parser* parser);
static ObjFunction* end_compiler(Parser* parser);
static void parse_precedence(GarbageCollector* gc, Parser* parser, Precedence precedence);
static ParseRule* get_rule(TokenType type);
static void expression(GarbageCollector* gc, Parser* parser);
static void declaration(GarbageCollector* gc, Parser* parser);
static void var_declaration(GarbageCollector* gc, Parser* parser, b32 immutable);
static void statement(GarbageCollector* gc, Parser* parser);
static void method(GarbageCollector* gc, Parser* parser);

static i32 resolve_local(Parser* parser, Compiler* compiler, Token* name, b32* immutable);
static i32 resolve_upvalue(Parser* parser, Compiler* compiler, Token* name, b32* immutable);
static Global* get_global(Parser* parser, Compiler* compiler, Token* name);
// =================================================================

#endif
