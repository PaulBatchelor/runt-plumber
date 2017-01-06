#include <stdio.h>
#include <stdlib.h>
#include <runt.h>
#include <soundpipe.h>
#include <sporth.h>
#include <string.h>
#include <jack/jack.h>

#include "audio.h"

static runt_int plumb_data(runt_vm *vm, user_data **ud)
{
    runt_int rc = RUNT_NOT_OK;
    runt_stacklet *s;
    runt_uint addr;
    user_data *udp;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    addr = s->f;

    rc = runt_memory_pool_get(vm, addr, (void **)&udp);
    RUNT_ERROR_CHECK(rc);
    *ud = udp;
    return rc;
}

static void process(sp_data *sp, void *udata){
    user_data *ud = udata;
    plumber_data *pd = &ud->pd;
    SPFLOAT out = 0;
    int chan;

    if(pd->recompile) {
        fprintf(stderr, "Recompiling!\n");
        plumber_recompile_string(&ud->pd, pd->str);
        pd->recompile = 0;
    }
    
    plumber_compute(pd, PLUMBER_COMPUTE);

    for (chan = 0; chan < pd->nchan; chan++) {
        out = sporth_stack_pop_float(&pd->sporth.stack);
        sp->out[chan] = out;
    }

    if(pd->showprog) {
        sp_progress_compute(sp, pd->prog, NULL, NULL);
    }
}

static runt_int plumb_new(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    runt_uint addr;
    user_data *ud;
    sp_data *sp;
    plumber_data *pd;

    size_t size = sizeof(user_data);
    addr = runt_malloc(vm, size, (void **)&ud);
    runt_mark_set(vm);

    sp_createn(&sp, 2);
    pd = &ud->pd; 
    pd->sp = sp;
    ud->sp = sp;
    plumber_register(pd);
    plumber_init(pd);
    /* plumber_parse_string(pd, "440 0.5 sine dup"); */
    pd->nchan = 2;

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);

    s->f = addr;

    return RUNT_OK;
}

static runt_int plumb_del(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;
    runt_stacklet *s;
    runt_uint addr;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    addr = s->f;

    rc = runt_memory_pool_get(vm, addr, (void **)&ud);
    RUNT_ERROR_CHECK(rc);

    ud->pd.fp = NULL;
    plumber_clean(&ud->pd);
    sp_destroy(&ud->sp);
    
    return RUNT_OK;
}

static runt_int plumb_start(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    start_audio(&ud->pd, ud, process, 6449);
    return RUNT_OK;
}

static runt_int plumb_stop(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    stop_audio(ud);
    return RUNT_OK;
}

static runt_int plumb_eval(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;
    runt_stacklet *s;
    const char *str;
   
    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);

    str = runt_to_string(s->p);

    plumber_parse_string(&ud->pd, (char *)str); 
    return RUNT_OK;
}

static runt_int plumb_init(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;
    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    plumber_compute(&ud->pd, PLUMBER_INIT);
    return RUNT_OK;
}

void runt_plugin_init(runt_vm *vm)
{
    runt_word_define(vm, "plumb_new", 9, plumb_new);
    runt_word_define(vm, "plumb_del", 9, plumb_del);
    runt_word_define(vm, "plumb_eval", 10, plumb_eval);
    runt_word_define(vm, "plumb_init", 10, plumb_init);
    runt_word_define(vm, "plumb_start", 11, plumb_start);
    runt_word_define(vm, "plumb_stop", 10, plumb_stop);
}
