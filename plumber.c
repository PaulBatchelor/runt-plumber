#include <stdio.h>
#include <stdlib.h>
#include <runt.h>
#include <soundpipe.h>
#include <sporth.h>
#include <string.h>
#include <jack/jack.h>
#include <stdlib.h>

#include "stream.h"
#include "data.h"
#include "runt_plumber.h"

void plumber_stop_jack(plumber_data *pd, int wait);

int plumber_set_var(plumber_data *pd, char *name, SPFLOAT *var);

static runt_int get_plumber_data(runt_vm *vm, plumber_data **pd)
{
    runt_int rc;
    runt_stacklet *s;

    rc = runt_ppop(vm, &s);
    *pd = runt_to_cptr(s->p);
    return rc;
}


static runt_int get_plumber_stream(runt_vm *vm, plumber_stream **stream)
{
    runt_int rc;
    runt_stacklet *s;

    rc = runt_ppop(vm, &s);
    *stream = runt_to_cptr(s->p);
    return rc;
}

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
    plumber_data *pd = ud->pd;
    SPFLOAT out = 0;
    int chan;

    if(pd->recompile) {
        plumber_recompile_stream(pd, ud->stream);
        pd->recompile = 0;
        /* clear the stream */
        plumber_stream_destroy(pd, ud->stream);
        plumber_stream_init(pd, ud->stream);
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
/*
static runt_int plumb_start(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    user_data *ud;
    plumber_data *pd;
    char *argv[] = {"sporth", "-b", "jack", "-c", "2", "-0", "-S"};

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    pd = ud->pd;

    sporth_run(pd, 7, argv, ud, process);
    return RUNT_OK;
}
*/
/*
static runt_int plumb_stop(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);

    plumber_stop_jack(ud->pd, 0);
    return RUNT_OK;
}
*/
static runt_int plumb_float(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    plumber_data *pd;
    plumber_stream *stream;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);

    plumber_stream_append_float(pd, stream, s->f);

    return RUNT_OK;
}

static runt_int plumb_string(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    user_data *ud;
    const char *str;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    str = runt_to_string(s->p);
    plumber_stream_append_string(ud->pd, ud->stream, str, strlen(str));
    return RUNT_OK;
}

static runt_int plumb_ugen(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    plumber_data *pd;
    plumber_stream *stream;
    const char *key;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    key = runt_to_string(s->p);

    rc = plumber_stream_append_ugen(pd, stream, key);

    if(rc == PLUMBER_NOTOK) {
        runt_print(vm, "Could not find ugen '%s'\n", key);
        return RUNT_NOT_OK;
    }

    return RUNT_OK;
}

/*
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
*/
/*
static runt_int plumb_clear(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    user_data *ud;
    plumber_stream *stream;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    stream = &ud->stream;

    plumber_stream_destroy(ud->pd, stream);
    plumber_stream_init(ud->pd, stream);

    return RUNT_OK;
}
*/

/*
static runt_int plumb_eval(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    user_data *ud;
    plumber_data *pd;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    pd = ud->pd;

    pd->recompile = 1;

    return rc;
}
*/

static runt_int plumb_new(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    plumber_data *pd;
    sp_data *sp;
    runt_int sr;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    sr = s->f;

    sp_create(&sp);
    sp->sr = sr;

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    
    pd = malloc(sizeof(plumber_data));
    s->p = runt_mk_cptr(vm, pd);

    plumber_register(pd);
    plumber_init(pd);
    pd->sp = sp;
    pd->log = vm->fp;

    pd->ud = vm;

    return RUNT_OK; 
}

static runt_int plumb_free(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    plumber_data *pd;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);

    plumber_clean(pd);
    sp_destroy(&pd->sp);
    free(pd);
    return RUNT_OK;
}
/*
static runt_int plumb_run(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    user_data *ud;
    plumber_data *pd;
    char *argv[] = {"sporth", "-b", "jack", "-c", "2", "-0"};

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    pd = ud->pd;

    sporth_run(pd, 6, argv, ud, process);
    return RUNT_OK;
}
*/
static runt_int plumb_parse(runt_vm *vm, runt_ptr p) 
{
    runt_int rc;
    runt_stacklet *s;
    const char *str;
    plumber_data *pd;
    plumber_stream *stream;
    
    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    str = runt_to_string(s->p);
 
    plumber_stream_parse_string(pd, stream, str);

    return RUNT_OK;
}

static runt_int plumb_var(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    const char *varname;
    runt_cell *cell;
    SPFLOAT *var;
    runt_uint id;
    plumber_data *pd;
    plumber_stream *stream;


    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    varname = runt_to_string(s->p);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    id = s->f;

    rc = runt_cell_pool_get_cell(vm, id + 1, &cell);
    RUNT_ERROR_CHECK(rc);
   
    var = (SPFLOAT *)cell->p.ud;

    plumber_stream_append_data(pd, stream, varname, strlen(varname), var, 0);

    plumber_ftmap_delete(pd, 0);
    plumber_ftmap_add_userdata(pd, varname, var);
    plumber_ftmap_delete(pd, 1);
    return RUNT_OK;
}

#if 0
static void plumb_define(runt_vm *vm, 
        runt_ptr p,
        const char *word,
        runt_uint size,
        runt_proc proc)
{
    runt_uint word_id;
    word_id = runt_word_define(vm, word, size, proc);
    runt_word_bind_ptr(vm, word_id, p);
}
#endif

static runt_int plumbstream_new(runt_vm *vm, runt_ptr p)
{
    plumber_data *pd;
    runt_int rc;
    plumber_stream *stream;
    runt_stacklet *s;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    
    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);

    stream = malloc(sizeof(plumber_stream));
    plumber_stream_init(pd, stream);

    s->p = runt_mk_cptr(vm, stream);
    return RUNT_OK;
}

static runt_int plumbstream_free(runt_vm *vm, runt_ptr p)
{
    plumber_stream *stream;
    plumber_data *pd;
    runt_int rc;
    
    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    plumber_stream_destroy(pd, stream);
    free(stream);
    
    return RUNT_OK;
}

static runt_int plumb_write(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    plumber_data *pd;
    plumber_stream *stream;
    const char *filename;
    runt_float dur;
    user_data ud;
    
    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    dur = s->f;
    
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    filename = runt_to_string(s->p);

    strncpy(pd->sp->filename, filename, 60);
    pd->sp->len = pd->sp->sr * dur;
    ud.pd = pd;
    ud.stream = stream;
    pd->recompile = 1;
    sp_process(pd->sp, &ud, process);

    return RUNT_OK;
}

static int sporth_tick(plumber_data *pd, sporth_stack *stack, void **ud)
{
    runt_vm *vm;
    runt_cell *cell;
    SPFLOAT in;

    if(pd->mode != PLUMBER_DESTROY) {
        in = sporth_stack_pop_float(stack);
    }

    if(pd->mode == PLUMBER_COMPUTE) {
        if(in != 0) {
            vm = pd->ud;
            cell = *ud;
            runt_cell_exec(vm, cell);
        }
    }
    return PLUMBER_OK;
}

static runt_int plumb_func(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    const char *fname;
    runt_int id;
    plumber_data *pd;
    plumber_stream *stream;
    runt_stacklet *s;
    runt_cell *cell;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    fname = runt_to_string(s->p);
    
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    id = s->f;

    runt_cell_id_get(vm, id, &cell);

    plumber_stream_append_function(pd, stream, 
            fname, strlen(fname), 
            sporth_tick, cell);

    return RUNT_OK;
}

runt_int runt_load_plumber(runt_vm *vm)
{
    /* runt_print(vm, "loading plumber...\n"); */
    /* user_data *ud; */
    /* runt_malloc(vm, sizeof(user_data), (void **)&ud); */
    /* runt_ptr p = runt_mk_cptr(vm, ud); */
    /* ud->magic = 123;
    ud->pd = &ud->ipd; */


    runt_word_define(vm, "plumb_new", 9, plumb_new);
    runt_word_define(vm, "plumb_free", 10, plumb_free);
    runt_word_define(vm, "plumbstream_new", 15, plumbstream_new);
    runt_word_define(vm, "plumbstream_free", 16, plumbstream_free);
    runt_word_define(vm, "plumb_float", 11, plumb_float);
    runt_word_define(vm, "plumb_string", 12, plumb_string);
    runt_word_define(vm, "plumb_ugen", 10, plumb_ugen);
    runt_word_define(vm, "plumb_write", 11, plumb_write);
    runt_word_define(vm, "plumb_parse", 11, plumb_parse);
    runt_word_define(vm, "plumb_var", 9, plumb_var);
    runt_word_define(vm, "plumb_func", 10, plumb_func);

    /*
    plumb_define(vm, p, "plumb_new", 9, plumb_new);
    plumb_define(vm, p, "plumb_parse", 11, plumb_parse);
    plumb_define(vm, p, "plumb_start", 11, plumb_start);
    plumb_define(vm, p, "plumb_stop", 10, plumb_stop);
    plumb_define(vm, p, "plumb_float", 11, plumb_float);
    plumb_define(vm, p, "plumb_string", 12, plumb_string);
    plumb_define(vm, p, "plumb_ugen", 10, plumb_ugen);
    plumb_define(vm, p, "plumb_print", 11, plumb_print);
    plumb_define(vm, p, "plumb_clear", 11, plumb_clear);
    plumb_define(vm, p, "plumb_eval", 10, plumb_eval);
    plumb_define(vm, p, "plumb_parse", 11, plumb_parse);
    plumb_define(vm, p, "plumb_run", 9, plumb_run);
    plumb_define(vm, p, "plumb_var", 9, plumb_var);
    */
    return RUNT_OK;
}


