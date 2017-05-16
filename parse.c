#include <stdio.h>
#include <runt.h>

runt_int runt_load_plumber(runt_vm *vm);

static runt_int loader(runt_vm *vm)
{
    runt_load_stdlib(vm);
    runt_load_plumber(vm);
    return RUNT_OK;
}

int main(int argc, char *argv[])
{
    return irunt_begin(argc, argv, loader);
}
