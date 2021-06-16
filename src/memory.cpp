#define GC_HEAP_GROW_FACTOR 2

void* reallocate(GarbageCollector* gc, void* pointer, size_t old_size, size_t new_size)
{
    gc->bytes_allocated += new_size - old_size;
    if (new_size > old_size)
    {
#ifdef DEBUG_STRESS_GC
        collect_garbage(gc);
#endif

        if (gc->bytes_allocated > gc->next_gc)
        {
            collect_garbage(gc);
        }
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

void mark_object(GarbageCollector* gc, Obj* object)
{
    if (object == NULL) return;
    if (object->is_marked) return;
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    print_value(obj_val(object));
    printf("\n");
#endif    
    object->is_marked = true;

    if (gc->gray_capacity < gc->gray_count + 1)
    {
        gc->gray_capacity = GROW_CAPACITY(gc->gray_capacity);
        gc->gray_stack    = (Obj**)realloc(gc->gray_stack, sizeof(Obj*) * gc->gray_capacity);
    }

    if(gc->gray_stack == NULL) exit(1);
    
    gc->gray_stack[gc->gray_count++] = object;
}

void mark_value(GarbageCollector* gc, Value value)
{
    if (is_obj(value)) mark_object(gc, value.as.obj);
}

static void mark_array(GarbageCollector* gc, ValueArray* array)
{
    for (i32 i = 0; i < array->count; i++)
    {
        mark_value(gc, array->values[i]);
    }
}

static void blacken_object(GarbageCollector* gc, Obj* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    print_value(obj_val(object));
    printf("\n");
#endif
    switch(object->type)
    {
        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)object;
            mark_object(gc, (Obj*)klass->name);
        }
        break;
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)object;
            mark_object(gc, (Obj*)closure->function);
            for (i32 i = 0; i < closure->upvalue_count; i++)
            {
                mark_object(gc, (Obj*)closure->upvalues[i]);
            }
        }
        break;
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)object;
            mark_object(gc, (Obj*)function->name);
            mark_array(gc, &function->chunk.constants);
        }
        break;
        case OBJ_UPVALUE:
        {
            mark_value(gc, ((ObjUpvalue*)object)->closed);
        }
        break;
        case OBJ_NATIVE:
        case OBJ_STRING:
        break;
    }
}

static void mark_roots(VM* vm)
{
    for(Value* slot = vm->stack; slot < vm->stack_top; slot++)
    {
        mark_value(&vm->gc, *slot);
    }

    for(i32 i = 0; i < vm->frame_count; i++)
    {
        mark_object(&vm->gc, (Obj*)vm->frames[i].closure);
    }

    for(ObjUpvalue* upvalue = vm->open_upvalues;
        upvalue != NULL; upvalue = upvalue->next)
    {
        mark_object(&vm->gc, (Obj*)upvalue);
    }

    mark_table(&vm->gc, &vm->globals);
    mark_compiler_roots(&vm->gc);
}

static void trace_references(GarbageCollector* gc)
{
    while (gc->gray_count > 0)
    {
        Obj* object = gc->gray_stack[--gc->gray_count];
        blacken_object(gc, object);
    }
}

static void sweep(GarbageCollector* gc, ObjectStore* store)
{
    Obj* previous = NULL;
    Obj* object   = store->objects;
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
                store->objects = object;
            }
            free_object(gc, unreached);
        }
    }
}

void collect_garbage(GarbageCollector* gc)
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = gc->bytes_allocated;
#endif
    mark_roots(gc->vm);
    trace_references(gc);
    table_remove_white(&gc->vm->strings);
    sweep(gc, &gc->vm->store);

    gc->next_gc = gc->bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - gc->bytes_allocated, before, gc->bytes_allocated,
           gc->next_gc);
#endif
}
