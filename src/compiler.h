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

using ParseFn = void(*)(Parser*);
struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

ParseRule rules[TOKEN_EOF];


// =================================================================
// API Functions
// =================================================================
bool compile(const char* source, Chunk* chunk);
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
static void emit_constant(Parser* parser);
static void end_compiler(Parser* parser);
static void grouping(Parser* parser);
static void number(Parser* parser);
static void binary(Parser* parser);
static void literal(Parser* parser);
static void unary(Parser* parser);
static void parse_precedence(Parser* parser, Precedence precedence);
static ParseRule* get_rule(TokenType type);
static void init_parse_rules();
static void expression(Parser* parser);
// =================================================================

#ifdef CLOX_COMPILER_IMPLEMENTATION

Chunk* compiling_chunk;
Chunk* current_chunk()
{
    return compiling_chunk;
}

static void error_at(Parser* parser, Token* token, const char* message)
{
    if(parser->panic_mode) return;
    
    parser->panic_mode = true;
    
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if(token->type != TOKEN_ERROR)
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->had_error = true;
}

static void error_at_current(Parser* parser, const char* message)
{
    error_at(parser, &parser->current, message);
}

static void error(Parser* parser, const char* message)
{
    error_at(parser, &parser->previous, message);
}

static void advance(Parser* parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = scan_token();
        if (parser->current.type != TOKEN_ERROR) break;

        error_at_current(parser, parser->current.start);
    }
}

static void consume(Parser* parser, TokenType type, const char* message)
{
    if (parser->current.type == type)
    {
        advance(parser);
        return;
    }

    error_at_current(parser, message);
}

static void emit_byte(Parser* parser, u8 byte)
{
    write_chunk(current_chunk(), byte, parser->previous.line);
}

static void emit_bytes(Parser* parser, u8 byte_1, u8 byte_2)
{
    emit_byte(parser, byte_1);
    emit_byte(parser,  byte_2);
}

static void emit_return(Parser* parser)
{
    emit_byte(parser, OP_RETURN);
}

static u8 make_constant(Parser* parser, Value value)
{
    i32 constant = add_constant(current_chunk(), value);
    if (constant > UINT8_MAX)
    {
        error(parser, "Too many constants in one chunk");
        return 0;
    }
    return (u8)constant;
}

static void emit_constant(Parser* parser, Value value)
{
    emit_bytes(parser, OP_CONSTANT, make_constant(parser, value));
}

static void end_compiler(Parser* parser)
{
#ifdef DEBUG_PRINT_CODE
    if (!parser->had_error)
    {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
    emit_return(parser);
}

static void binary(Parser* parser)
{
    TokenType operator_type = parser->previous.type;

    ParseRule* rule = get_rule(operator_type);
    parse_precedence(parser, (Precedence)(rule->precedence + 1));

    switch(operator_type)
    {
    case TOKEN_BANG_EQUAL:        emit_bytes(parser, OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:       emit_byte(parser, OP_EQUAL); break;
    case TOKEN_GREATER:           emit_byte(parser, OP_GREATER); break;
    case TOKEN_GREATER_EQUAL:     emit_bytes(parser, OP_LESS, OP_NOT); break;
    case TOKEN_LESS:              emit_byte(parser, OP_LESS); break;
    case TOKEN_LESS_EQUAL:        emit_bytes(parser, OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:              emit_byte(parser, OP_ADD); break;
    case TOKEN_MINUS:             emit_byte(parser, OP_SUBTRACT); break;
    case TOKEN_STAR:              emit_byte(parser, OP_MULTIPLY); break;
    case TOKEN_SLASH:             emit_byte(parser, OP_DIVIDE); break;
    default:
    return;
    }
}
static void literal(Parser* parser)
{
    switch(parser->previous.type)
    {
    case TOKEN_FALSE: emit_byte(parser, OP_FALSE); break;
    case TOKEN_NIL: emit_byte(parser, OP_NIL); break;
    case TOKEN_TRUE: emit_byte(parser, OP_TRUE); break;
    default:
    return;
    }
}

static void grouping(Parser* parser)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");            
}

static void number(Parser* parser)
{
    f64 value = strtod(parser->previous.start, NULL);
    emit_constant(parser, number_val(value));
}

static void unary(Parser* parser)
{
    TokenType operator_type = parser->previous.type;

    parse_precedence(parser, PREC_UNARY);

    switch(operator_type)
    {
    case TOKEN_BANG: emit_byte(parser, OP_NOT); break;
    case TOKEN_MINUS: emit_byte(parser, OP_NEGATE); break;
    default:
    return;
    }
}

static void parse_precedence(Parser* parser, Precedence precedence)
{
    advance(parser);
    ParseFn prefix_rule = get_rule(parser->previous.type)->prefix;
    if (prefix_rule == NULL)
    {
        error(parser, "Expect expression");
        return;
    }
    prefix_rule(parser);

    while (precedence <= get_rule(parser->current.type)->precedence)
    {
        advance(parser);
        ParseFn infix_rule = get_rule(parser->previous.type)->infix;
        infix_rule(parser);
    }
}

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

static void expression(Parser* parser)
{
    parse_precedence(parser, PREC_ASSIGNMENT);
}

static void init_parse_rules()
{
    rules[TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE};
    rules[TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}; 
    rules[TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_MINUS]         = {unary,    binary, PREC_TERM};
    rules[TOKEN_PLUS]          = {NULL,     binary, PREC_TERM};
    rules[TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR};
    rules[TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR};
    rules[TOKEN_BANG]          = {unary,    NULL,   PREC_NONE};
    rules[TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_NONE};
    rules[TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY};
    rules[TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON};
    rules[TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON};
    rules[TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON};
    rules[TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON};
    rules[TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE};
    rules[TOKEN_AND]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE};
    rules[TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_IF]            = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_NIL]           = {literal,  NULL,   PREC_NONE};
    rules[TOKEN_OR]            = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE};
    rules[TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE};
}

bool compile(const char* source, Chunk* chunk)
{
    init_scanner(source);
    compiling_chunk = chunk;

    Parser parser = {};
    advance(&parser);
    expression(&parser);
    consume(&parser, TOKEN_EOF, "Expect end of expression.");

    end_compiler(&parser);
    return !parser.had_error;
}

#endif

#endif
