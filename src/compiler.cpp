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

static bool check(Parser* parser, TokenType type)
{
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type)
{
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
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

static void binary(Parser* parser, bool can_assign)
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

static void literal(Parser* parser, bool can_assign)
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

static void grouping(Parser* parser, bool can_assign)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");            
}

static void number(Parser* parser, bool can_assign)
{
    f64 value = strtod(parser->previous.start, NULL);
    emit_constant(parser, number_val(value));
}

static void string(Parser* parser, bool can_assign)
{
    emit_constant(parser, obj_val(copy_string(parser->store, parser->strings, parser->previous.start + 1,
                                              parser->previous.length - 2)));
}

static void named_variable(Parser* parser, Token name, bool can_assign)
{
    u8 arg = identifier_constant(parser, &name);

    if (can_assign && match(parser, TOKEN_EQUAL))
    {
        expression(parser);
        emit_bytes(parser, OP_SET_GLOBAL, arg);
    }
    else
    {
        emit_bytes(parser, OP_GET_GLOBAL, arg);
    }
}

static void variable(Parser* parser, bool can_assign)
{
    named_variable(parser, parser->previous, can_assign);
}

static void unary(Parser* parser, bool can_assign)
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
    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(parser, can_assign);

    while (precedence <= get_rule(parser->current.type)->precedence)
    {
        advance(parser);
        ParseFn infix_rule = get_rule(parser->previous.type)->infix;
        infix_rule(parser, can_assign);
    }

    if (can_assign && match(parser, TOKEN_EQUAL))
    {
        error(parser, "Invalid assignment target.");
    }
}

static u8 identifier_constant(Parser* parser, Token* name)
{
    return make_constant(parser, obj_val(copy_string(parser->store, parser->strings, name->start,
                                                     name->length)));
}

static u8 parse_variable(Parser* parser, const char* error_message)
{
    consume(parser, TOKEN_IDENTIFIER, error_message);
    return identifier_constant(parser, &parser->previous);
}

static void define_variable(Parser* parser, u8 global)
{
    emit_bytes(parser, OP_DEFINE_GLOBAL, global);
}

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

static void expression(Parser* parser)
{
    parse_precedence(parser, PREC_ASSIGNMENT);
}

static void expression_statement(Parser* parser)
{
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(parser, OP_POP);
}

static void var_declaration(Parser* parser)
{
    u8 global = parse_variable(parser, "Expect variable name.");

    if (match(parser, TOKEN_EQUAL))
    {
        expression(parser);
    }
    else
    {
        emit_byte(parser, OP_NIL);
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declarataion.");
    define_variable(parser, global);
}

static void print_statement(Parser* parser)
{
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(parser, OP_PRINT);
}

static void synchronize(Parser* parser)
{
    parser->panic_mode = false;

    while (parser->current.type != TOKEN_EOF)
    {
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        switch (parser->current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
        return;
        default:
        // @Note: Do nothing
        ;
        }

        advance(parser);
    }
}

static void declaration(Parser* parser)
{
    if (match(parser, TOKEN_VAR))
    {
        var_declaration(parser);
    }
    else
    {
        statement(parser);
    }

    if (parser->panic_mode) synchronize(parser);
}

static void statement(Parser* parser)
{
    if (match(parser, TOKEN_PRINT))
    {
        print_statement(parser);
    }
    else
    {
        expression_statement(parser);
    }
}

void init_parse_rules()
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
    rules[TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE};
    rules[TOKEN_STRING]        = {string,   NULL,   PREC_NONE};
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

bool compile(const char* source, Chunk* chunk, ObjectStore* output_store, Table* output_strings)
{
    init_scanner(source);
    compiling_chunk = chunk;

    Parser parser = {};
    parser.store = output_store;
    parser.strings = output_strings;
    advance(&parser);

    while (!match(&parser, TOKEN_EOF))
    {
        declaration(&parser);
    }
  
    end_compiler(&parser);
    return !parser.had_error;
}
