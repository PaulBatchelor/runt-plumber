#include <stdio.h>
#include <stdlib.h>
#include <runt.h>
#include <soundpipe.h>
#include <sporth.h>
#include <string.h>
#include <stdlib.h>

#include "plumbstream.h"
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

    for (chan = 0; chan < pd->sp->nchan; chan++) {
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
    plumber_data *pd;
    plumber_stream *stream;
    const char *str;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    str = runt_to_string(s->p);
    plumber_stream_append_string(pd, stream, str, strlen(str));
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

    if(pd->showprog) {
        sp_progress_destroy(&pd->prog);
    }
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

    plumber_stream_append_data(pd, 
        stream, varname, strlen(varname), var, 0, PTYPE_USERDATA);

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
    pd->recompile = 0; 
    if(pd->showprog) {
        sp_progress_init(pd->sp, pd->prog);
    }
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

static runt_int plumb_udata(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    const char *str;
    plumber_data *pd;
    void *ud;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    str = runt_to_string(s->p);

    if(plumber_ftmap_search_userdata(pd, str, &ud) == PLUMBER_NOTOK) {
        runt_print(vm, "plumb_udata: could not find table '%s'\n", str);
        return RUNT_NOT_OK;
    }

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->p = runt_mk_cptr(vm, ud);
    return RUNT_OK;
}

static runt_int plumb_compile(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    plumber_data *pd;
    plumber_stream *stream;
    
    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    plumber_recompile_stream(pd, stream);
    return RUNT_OK;
}

static runt_int plumb_ftbl(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    const char *ftname;
    sp_ftbl *ft;
    plumber_data *pd;
    
    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    ftname = runt_to_string(s->p);

    rc = plumber_ftmap_search(pd, ftname, &ft);
    if(rc != PLUMBER_OK) {
        runt_print(vm, "Could not find ftable %s\n", ftname);
        return RUNT_NOT_OK;
    }

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->p = runt_mk_cptr(vm, ft);

    return RUNT_OK;
}

static runt_int plumb_ftmap(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s1;
    runt_stacklet *s2;
    runt_uint id;
    sp_ftbl *ft;
    runt_uint i;
    
    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    ft = runt_to_cptr(s1->p);
    
    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    id = s1->f;

    for(i = 0; i < ft->size; i++) {
        rc = runt_ppush(vm, &s1);
        RUNT_ERROR_CHECK(rc);
        rc = runt_ppush(vm, &s2);
        RUNT_ERROR_CHECK(rc);

        s1->f = i;
        s2->f = ft->tbl[i];
        runt_cell_id_exec(vm, id);

        rc = runt_ppop(vm, &s1);
        RUNT_ERROR_CHECK(rc);
        ft->tbl[i] = s1->f;
    }
    return RUNT_OK;
}

static runt_int plumb_ftsize(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    runt_uint size;
    sp_ftbl *ft;
    
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    ft = runt_to_cptr(s->p);
   
    size = ft->size;

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->f = size;
    return RUNT_OK;
}

static runt_int plumb_append(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    plumber_data *pd;
    plumber_stream *stream;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);

    plumber_stream_append_to_main(pd, stream);

    return RUNT_OK;
}

static runt_int plumb_prog(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    plumber_data *pd;
    plumber_stream *stream;

    rc = get_plumber_data(vm, &pd);
    RUNT_ERROR_CHECK(rc);
    rc = get_plumber_stream(vm, &stream);
    RUNT_ERROR_CHECK(rc);
    
    pd->showprog = 1;

    sp_progress_create(&pd->prog);
    return RUNT_OK;
}

runt_int runt_load_plumber(runt_vm *vm)
{
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
    runt_word_define(vm, "plumb_udata", 11, plumb_udata);
    runt_word_define(vm, "plumb_compile", 13, plumb_compile);
    runt_word_define(vm, "plumb_ftbl", 10, plumb_ftbl);
    runt_word_define(vm, "plumb_ftmap", 11, plumb_ftmap);
    runt_word_define(vm, "plumb_ftsize", 12, plumb_ftsize);
    runt_word_define(vm, "plumb_append", 12, plumb_append);
    runt_word_define(vm, "plumb_prog", 10, plumb_prog);
    return RUNT_OK;
}


