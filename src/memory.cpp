void* reallocate(void* pointer, size_t old_size, size_t new_size)
{
    if (new_size > old_size)
    {
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#endif
    }

    if (new_size == 0)
    {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) exit(1);
    return result;
}

void mark_object(Obj* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    print_value(obj_val(object));
    printf("\n");
#endif
    if (object == NULL) return;
    object->is_marked = true;
}

void mark_value(Value value)
{
    if (is_obj(value)) mark_object(value.as.obj);
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif
    mark_roots();

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

static void mark_roots()
{
    for(Value* slot = vm->stack; slot < vm->stack_top; slot++)
    {
        mark_value(*slot);
    }

    for(i32 i = 0; i < vm->frame_count; i++)
    {
        mark_object((Obj*)vm->frames[i].closure);
    }

    for(ObjUpvalue* upvalue = vm->open_upvalues;
        upvalue != NULL; upvalue = upvalue->next)
    {
        mark_object((Obj*)upvalue);
    }

    mark_table(&vm->globals);
    mark_compiler_roots();
}
