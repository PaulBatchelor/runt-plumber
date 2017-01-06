#include <stdio.h>
#include <stdlib.h>
#include <runt.h>
#include <soundpipe.h>
#include <sporth.h>
#include <string.h>
#include <jack/jack.h>
#include <stdlib.h>

#include "audio.h"
#include "stream.h"
#include "data.h"

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
        plumber_recompile_stream(&ud->pd, &ud->stream);
        pd->recompile = 0;
        /* clear the stream */
        plumber_stream_destroy(&ud->pd, &ud->stream);
        plumber_stream_init(&ud->pd, &ud->stream);
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
    plumber_parse_string(pd, "0 0");
    plumber_compute(pd, PLUMBER_INIT);
    pd->nchan = 2;

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);

    s->f = addr;

    /* initialize plumber stream */
    plumber_stream_init(pd, &ud->stream);

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
    runt_print(vm, "Starting up audio engine...\n");
    start_audio(&ud->pd, ud, process, 6449);
    return RUNT_OK;
}

static runt_int plumb_stop(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    runt_print(vm, "Stopping audio engine...\n");
    stop_audio(&ud->jd);
    return RUNT_OK;
}

static runt_int plumb_float(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    user_data *ud;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);

    plumber_stream_append_float(&ud->pd, &ud->stream, s->f);

    return RUNT_OK;
}

static runt_int plumb_ugen(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    user_data *ud;
    const char *key;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    key = runt_to_string(s->p);

    rc = plumber_stream_append_ugen(&ud->pd, &ud->stream, key);

    if(rc == PLUMBER_NOTOK) {
        runt_print(vm, "Could not find ugen '%s'\n", key);
        return RUNT_NOT_OK;
    }

    return RUNT_OK;
}

static runt_int plumb_print(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    user_data *ud;
    runt_int i;
    plumber_stream *stream;
    plumber_word *word;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    stream = &ud->stream;

    word = stream->root.next;
    for(i = 0; i < stream->size; i++) {
        runt_print(vm, "%d\t%g\n", word->type, word->flt);
        word = word->next;
    }

    return RUNT_OK;
}

static runt_int plumb_clear(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    user_data *ud;
    plumber_stream *stream;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    stream = &ud->stream;

    plumber_stream_destroy(&ud->pd, stream);
    /* reinitialize */
    plumber_stream_init(&ud->pd, stream);

    return RUNT_OK;
}

static runt_int plumb_eval(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    user_data *ud;
    plumber_data *pd;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    pd = &ud->pd;

    pd->recompile = 1;
    /*rc = plumber_recompile_stream(&ud->pd, stream);*/

    return rc;
}

void runt_plugin_init(runt_vm *vm)
{
    runt_word_define(vm, "plumb_new", 9, plumb_new);
    runt_word_define(vm, "plumb_del", 9, plumb_del);
    runt_word_define(vm, "plumb_start", 11, plumb_start);
    runt_word_define(vm, "plumb_stop", 10, plumb_stop);
    runt_word_define(vm, "plumb_float", 11, plumb_float);
    runt_word_define(vm, "plumb_ugen", 10, plumb_ugen);
    runt_word_define(vm, "plumb_print", 11, plumb_print);
    runt_word_define(vm, "plumb_clear", 11, plumb_clear);
    runt_word_define(vm, "plumb_eval", 10, plumb_eval);
}
