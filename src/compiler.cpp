Compiler* current = NULL;

Chunk* current_chunk()
{
    return &current->function->chunk;
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

static b32 check(Parser* parser, TokenType type)
{
    return parser->current.type == type;
}

static b32 match(Parser* parser, TokenType type)
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
    emit_byte(parser, byte_2);
}

static void emit_loop(Parser* parser, i32 loop_start)
{
    emit_byte(parser, OP_LOOP);

    i32 offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) error(parser, "Loop body too large.");

    emit_byte(parser, (offset >> 8) & 0xff);
    emit_byte(parser, offset & 0xff);
}

static i32 emit_jump(Parser* parser, u8 instruction)
{
    emit_byte(parser, instruction);
    emit_byte(parser, 0xff);
    emit_byte(parser, 0xff);
    return current_chunk()->count - 2;
}

static void emit_return(Parser* parser)
{
    emit_byte(parser, OP_NIL);
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

static void patch_jump(Parser* parser, i32 offset)
{
    i32 jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX)
    {
        error(parser, "Too much code to jump over.");
    }

    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

static void init_compiler(Compiler* compiler, Parser* parser, FunctionType type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function = new_function(parser->store);
    current = compiler;

    if (type != TYPE_SCRIPT)
    {
        current->function->name = copy_string(parser->store, parser->strings, parser->previous.start, parser->previous.length);
    }

    Local* local = &current->locals[current->local_count++];
    local->depth       = 0;
    local->is_captured = false;
    local->name.start  = "";
    local->name.length = 0;
}

static ObjFunction* end_compiler(Parser* parser)
{
    ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser->had_error)
    {
        disassemble_chunk(current_chunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif
    emit_return(parser);
    current = current->enclosing;

    return function;
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope(Parser* parser)
{
    current->scope_depth--;

    while (current->local_count > 0 &&
           current->locals[current->local_count - 1].depth > current->scope_depth)
    {        
        if(current->locals[current->local_count - 1].is_captured)
        {
            emit_byte(parser, OP_CLOSE_UPVALUE);
        }
        else
        {
            emit_byte(parser, OP_POP);
        }
        
        current->local_count--;
    }
}

static void binary(Parser* parser, b32 can_assign)
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

static u8 argument_list(Parser* parser)
{
    u8 arg_count = 0;
    if (!check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            expression(parser);
            if (arg_count == 255)
            {
                error(parser, "Can't have more than 255 arguments.");
            }
            arg_count++;
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

static void call(Parser* parser, b32 can_assign)
{
    u8 arg_count = argument_list(parser);
    emit_bytes(parser, OP_CALL, arg_count);
}

static void literal(Parser* parser, b32 can_assign)
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

static void grouping(Parser* parser, b32 can_assign)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");            
}

static void number(Parser* parser, b32 can_assign)
{
    f64 value = strtod(parser->previous.start, NULL);
    emit_constant(parser, number_val(value));
}

static void or_(Parser* parser, b32 can_assign)
{
    i32 else_jump = emit_jump(parser, OP_JUMP_IF_FALSE);
    i32 end_jump = emit_jump(parser, OP_JUMP);

    patch_jump(parser, else_jump);
    emit_byte(parser, OP_POP);

    parse_precedence(parser, PREC_OR);
    patch_jump(parser, end_jump);
}

static void string(Parser* parser, b32 can_assign)
{
    emit_constant(parser, obj_val(copy_string(parser->store, parser->strings, parser->previous.start + 1,
                                              parser->previous.length - 2)));
}

static void named_variable(Parser* parser, Token name, b32 can_assign)
{
    u8 get_op, set_op;
    b32 immutable = false;
    i32 arg = resolve_local(parser, current, &name, &immutable);
    if (arg != -1)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    else if ((arg = resolve_upvalue(parser, current, &name, &immutable)) != -1)
    {
        get_op = OP_GET_UPVALUE;
        set_op = OP_SET_UPVALUE;
    }
    else
    {
        Global* global = get_global(parser, current, &name);
        if (global)
        {
            immutable = global->immutable;
        }
        arg = identifier_constant(parser, &name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    
    if (can_assign && !immutable && match(parser, TOKEN_EQUAL))
    {
        expression(parser);
        emit_bytes(parser, set_op, (u8)arg);
    }
    else
    {
        emit_bytes(parser, get_op, (u8)arg);
    }
}

static void variable(Parser* parser, b32 can_assign)
{
    named_variable(parser, parser->previous, can_assign);
}

static void unary(Parser* parser, b32 can_assign)
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
    
    b32 can_assign = precedence <= PREC_ASSIGNMENT;
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

static b32 identifiers_equal(Token* a, Token* b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static Global* get_global(Parser* parser, Compiler* compiler, Token* name)
{
    for (i32 i = global_count - 1; i >= 0; i--)
    {
        Global* global = &globals[i];
        if (strncmp(name->start, global->name->chars, global->name->length) == 0)
        {
            return global;
        }
    }

    return NULL;
}

static i32 resolve_local(Parser* parser, Compiler* compiler, Token* name, b32* immutable)
{
    for (i32 i = compiler->local_count - 1; i >= 0; i--)
    {
        Local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name))
        {
            if (local->depth == -1)
            {
                error(parser, "Can't read local variable in its own initializer.");
            }
            *immutable = local->immutable;
            return i;
        }
    }
    return -1;
}

static i32 add_upvalue(Parser* parser, Compiler* compiler, u8 index, b32 is_local)
{
    i32 upvalue_count = compiler->function->upvalue_count;    
    for(i32 i = 0; i < upvalue_count; i++)
    {
        Upvalue* upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->is_local == is_local)
        {
            return i;
        }
    }

    if(upvalue_count == UINT8_COUNT)
    {
        error(parser, "Too many closure variables in function.");
        return 0;
    }
    
    compiler->upvalues[upvalue_count].is_local = is_local;
    compiler->upvalues[upvalue_count].index    = index;
    return compiler->function->upvalue_count++;
}

static i32 resolve_upvalue(Parser* parser, Compiler* compiler, Token* name, b32* immutable)
{
    if(compiler->enclosing == NULL) return -1;

    i32 local = resolve_local(parser, compiler->enclosing, name, immutable);
    if(local != -1)
    {
        compiler->enclosing->locals[local].is_captured = true;
        return add_upvalue(parser, compiler, (u8)local, true);
    }

    i32 upvalue = resolve_upvalue(parser, compiler->enclosing, name, immutable);
    if(upvalue != -1)
    {
        return add_upvalue(parser, compiler, (u8)upvalue, false);
    }
    
    return -1;
}

static void add_local(Parser* parser, Token name, b32 immutable)
{
    if (current->local_count == UINT8_COUNT)
    {
        error(parser, "Too many local variables in function.");
        return;
    }
    
    Local* local = &current->locals[current->local_count++];
    local->name        = name;
    local->depth       = -1;
    local->is_captured = false;
    local->immutable   = immutable;
}

static void add_global(Parser* parser, Token name, b32 immutable)
{
    if (global_count == UINT8_COUNT)
    {
        error(parser, "Too many global variables defined.");
        return;
    }

    Global* global = &globals[global_count++];
    global->name = copy_string(parser->store, parser->strings, name.start, name.length);
    global->immutable = immutable;
}

static void declare_variable(Parser* parser, b32 immutable)
{
    Token* name = &parser->previous;
    if (current->scope_depth == 0)
    {
        for (i32 i = global_count - 1; i >= 0; i--)
        {
            Global* global = &globals[i];
            if (strncmp(name->start, global->name->chars, global->name->length) == 0)
            {
                error(parser, "Already global with this name.");
            }
        }
        add_global(parser, *name, immutable);
    }
    else
    {
        for (i32 i = current->local_count - 1; i >= 0; i--)
        {
            Local* local = &current->locals[i];
            if (local->depth != -1 && local->depth < current->scope_depth)
            {
                break;
            }

            if (identifiers_equal(name, &local->name))
            {
                error(parser, "Already variable with this name in this scope.");
            }
        }
        add_local(parser, *name, immutable);
    }
}

static u8 parse_variable(Parser* parser, const char* error_message, b32 immutable)
{
    consume(parser, TOKEN_IDENTIFIER, error_message);

    declare_variable(parser, immutable);
    if (current->scope_depth > 0) return 0;

    return identifier_constant(parser, &parser->previous);
}

static void mark_initialized()
{
    if (current->scope_depth == 0) return;
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void define_variable(Parser* parser, u8 global)
{
    if (current->scope_depth > 0)
    {
        mark_initialized();
        return;
    }
    
    emit_bytes(parser, OP_DEFINE_GLOBAL, global);
}

static void and_(Parser* parser, b32 can_assign)
{
    i32 end_jump = emit_jump(parser, OP_JUMP_IF_FALSE);

    emit_byte(parser, OP_POP);
    parse_precedence(parser, PREC_AND);

    patch_jump(parser, end_jump);
}

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

static void expression(Parser* parser)
{
    parse_precedence(parser, PREC_ASSIGNMENT);
}

static void block(Parser* parser)
{
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF))
    {
        declaration(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(Parser* parser, FunctionType type)
{
    Compiler compiler = {};
    init_compiler(&compiler, parser, type);
    begin_scope();

    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            current->function->arity++;
            if (current->function->arity > 255)
            {
                error_at_current(parser, "Can't have more than 255 parameters.");
            }

            u8 param_constant = parse_variable(parser, "Expect parameter name.", true); // @Note: Check for default values?
            define_variable(parser, param_constant);
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block(parser);

    ObjFunction* function = end_compiler(parser);
    emit_bytes(parser, OP_CLOSURE, make_constant(parser, obj_val(function)));

    for(i32 i = 0; i < function->upvalue_count; i++)
    {
        emit_byte(parser, compiler.upvalues[i].is_local ? 1 : 0);
        emit_byte(parser, compiler.upvalues[i].index);
    }
}

static void fun_declaration(Parser* parser)
{
    u8 global = parse_variable(parser, "Expect function name.", true);
    mark_initialized();
    function(parser, TYPE_FUNCTION);
    define_variable(parser, global);
}

static void expression_statement(Parser* parser)
{
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(parser, OP_POP);
}

static void for_statement(Parser* parser)
{
    begin_scope();
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(parser, TOKEN_SEMICOLON))
    {
        
    }
    else if (match(parser, TOKEN_LET))
    {
        var_declaration(parser, false);
    }
    else if (match(parser, TOKEN_CONST))
    {
        var_declaration(parser, true);
    }
    else
    {
        expression_statement(parser);
    }

    i32 loop_start = current_chunk()->count;

    i32 exit_jump = -1;

    if (!match(parser, TOKEN_SEMICOLON))
    {
        expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exit_jump = emit_jump(parser, OP_JUMP_IF_FALSE);
        emit_byte(parser, OP_POP);
    }

    if (!match(parser, TOKEN_RIGHT_PAREN))
    {
        i32 body_jump = emit_jump(parser, OP_JUMP);

        i32 increment_start = current_chunk()->count;
        expression(parser);
        emit_byte(parser, OP_POP);
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(parser, loop_start);
        loop_start = increment_start;
        patch_jump(parser, body_jump);
    }       

    statement(parser);

    emit_loop(parser, loop_start);

    if (exit_jump != -1)
    {
        patch_jump(parser, exit_jump);
        emit_byte(parser, OP_POP);
    }

    end_scope(parser);
}

static void switch_statement(Parser* parser)
{
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after switch condition.");

    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' after switch expression.");

    i32 exit_jumps[64];
    i32 exit_jump_count = 0;

    while (match(parser, TOKEN_CASE))
    {
        // Parse the case value
        expression(parser);

        // consume colon
        consume(parser, TOKEN_COLON, "Expect ':' after case value.");

        // Check for equality
        emit_byte(parser, OP_COMPARE);
        i32 then_jump = emit_jump(parser, OP_JUMP_IF_FALSE);

        // If false
        emit_byte(parser, OP_POP); // pop the value
        emit_byte(parser, OP_POP);
        statement(parser);

        exit_jumps[exit_jump_count++] = emit_jump(parser, OP_JUMP);

        patch_jump(parser, then_jump);
        emit_byte(parser, OP_POP); // pop the value
        emit_byte(parser, OP_POP);
    }

    if (match(parser, TOKEN_DEFAULT))
    {
        consume(parser, TOKEN_COLON, "Expect ':' after default label.");
        statement(parser);
    }

    for (i32 i = 0; i < exit_jump_count; i++)
    {
        patch_jump(parser, exit_jumps[i]);
    }

    emit_byte(parser, OP_POP);

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after switch statement.");
}

static void if_statement(Parser* parser)
{
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    i32 then_jump = emit_jump(parser, OP_JUMP_IF_FALSE);
    emit_byte(parser, OP_POP);
    statement(parser);
    i32 else_jump = emit_jump(parser, OP_JUMP);

    patch_jump(parser, then_jump);

    emit_byte(parser, OP_POP);

    if (match(parser, TOKEN_ELSE)) statement(parser);
    patch_jump(parser, else_jump);
}

static void var_declaration(Parser* parser, b32 immutable)
{
    u8 global = parse_variable(parser, "Expect variable name.", immutable);

    if (match(parser, TOKEN_EQUAL))
    {
        expression(parser);
    }
    else if (immutable && !match(parser, TOKEN_EQUAL))
    {
        error(parser, "Const variables must be initialized.");
    }
    else 
    {
        emit_byte(parser, OP_NIL);
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    define_variable(parser, global);
}

static void print_statement(Parser* parser)
{
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(parser, OP_PRINT);
}

static void return_statement(Parser* parser)
{
    if (current->type == TYPE_SCRIPT)
    {
        error(parser, "Can't return from top-level code.");
    }
    
    if (match(parser, TOKEN_SEMICOLON))
    {
        emit_return(parser);
    }
    else
    {
        expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value.");
        emit_byte(parser, OP_RETURN);
    }
}

static void while_statement(Parser* parser)
{
    i32 loop_start = current_chunk()->count;
    
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    i32 exit_jump = emit_jump(parser, OP_JUMP_IF_FALSE);

    emit_byte(parser, OP_POP);
    statement(parser);

    emit_loop(parser, loop_start);

    patch_jump(parser, exit_jump);
    emit_byte(parser, OP_POP);
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
        case TOKEN_LET:
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
    if (match(parser, TOKEN_FUN))
    {
        fun_declaration(parser);
    }
    else if (match(parser, TOKEN_LET))
    {
        var_declaration(parser, false);
    }
    else if (match(parser, TOKEN_CONST))
    {
        var_declaration(parser, true);
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
    else if (match(parser, TOKEN_IF))
    {
        if_statement(parser);
    }
    else if (match(parser, TOKEN_RETURN))
    {
        return_statement(parser);
    }
    else if (match(parser, TOKEN_WHILE))
    {
        while_statement(parser);
    }
    else if (match(parser, TOKEN_FOR))
    {
        for_statement(parser);
    }
    else if (match(parser, TOKEN_SWITCH))
    {
        switch_statement(parser);
    }
    else if (match(parser, TOKEN_LEFT_BRACE))
    {
        begin_scope();
        block(parser);
        end_scope(parser);
    }
    else
    {
        expression_statement(parser);
    }
}

void init_parse_rules()
{
    rules[TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL};
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
    rules[TOKEN_AND]           = {NULL,     and_,   PREC_NONE};
    rules[TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE};
    rules[TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_IF]            = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_NIL]           = {literal,  NULL,   PREC_NONE};
    rules[TOKEN_OR]            = {NULL,     or_,    PREC_NONE};
    rules[TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE};
    rules[TOKEN_LET]           = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_CONST]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE};
    rules[TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE};
}

ObjFunction* compile(const char* source, ObjectStore* output_store, Table* output_strings)
{
    init_scanner(source);

    Parser parser = {};
    parser.store = output_store;
    parser.strings = output_strings;

    Compiler compiler = {};
    init_compiler(&compiler, &parser, TYPE_SCRIPT);

    advance(&parser);

    while (!match(&parser, TOKEN_EOF))
    {
        declaration(&parser);
    }
  
    ObjFunction* function = end_compiler(&parser);
    return parser.had_error ? NULL : function;
}
