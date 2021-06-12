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
    if (object == NULL) return;
    if (object->is_marked) return;
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    print_value(obj_val(object));
    printf("\n");
#endif    
    object->is_marked = true;

    if (vm->gray_capacity < vm->gray_count + 1)
    {
        vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
        vm->gray_stack    = (Obj**)realloc(vm->gray_stack, sizeof(Obj*) * vm->gray_capacity);
    }

    if(vm->gray_stack == NULL) exit(1);
    
    vm->gray_stack[vm->gray_count++] = object;
}

void mark_value(Value value)
{
    if (is_obj(value)) mark_object(value.as.obj);
}

static void mark_array(ValueArray* array)
{
    for (i32 i = 0; i < array->count; i++)
    {
        mark_value(array->values[i]);
    }
}

static void blacken_object(Obj* object)
{
#if DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif
    switch(object->type)
    {
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)object;
            mark_object((Obj*)closure->function);
            for (i32 i = 0; i < closure->upvalue_count; i++)
            {
                mark_object((Obj*)closure->upvalues[i]);
            }
        }
        break;
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)object;
            mark_object((Obj*)function->name);
            mark_array(&function->chunk.constants);
        }
        break;
        case OBJ_UPVALUE:
        {
            mark_value(((ObjUpvalue*)object)->closed);
        }
        break;
        case OBJ_NATIVE:
        case OBJ_STRING:
        break;
    }
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif
    mark_roots();
    trace_references();
    table_remove_white(&vm->strings);
    sweep();

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

static void trace_references()
{
    while (vm->gray_count > 0)
    {
        Obj* object = vm->gray_stack[--vm->gray_count];
        blacken_object(object);
    }
}

static void sweep()
{
    Obj* previous = NULL;
    Obj* object   = vm->objects;
    while (object != NULL)
    {
        if (object->is_marked)
        {
            object->is_marked = false;
            previous = object;
            object   = object->next;
        }
        else
        {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL)
            {
                previous->next = object;
            }
            else
            {
                vm->objects = object;
            }
        }

        free_object(unreached);
    }
}
