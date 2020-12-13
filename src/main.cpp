#define CLOX_IMPLEMENTATION
#include "clox.h"

int main(int argc, const char* argv[])
{
    VM vm = {};

    init_vm(&vm);
    init_parse_rules();

    if (argc == 1)
    {
        repl(&vm);
    }
    else if(argc == 2)
    {
        run_file(&vm, argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    free_vm(&vm);
    return 0;
}
