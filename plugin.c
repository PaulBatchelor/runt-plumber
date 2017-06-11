#include <stdlib.h>
#include <stdio.h>
#include <runt.h>
#include <soundpipe.h>
#include <sporth.h>
#include "runt_plumber.h"

void runt_plugin_init(runt_vm *vm)
{
    runt_load_plumber(vm);
}
