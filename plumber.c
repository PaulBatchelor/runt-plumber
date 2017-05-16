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
        plumber_recompile_stream(pd, &ud->stream);
        pd->recompile = 0;
        /* clear the stream */
        plumber_stream_destroy(pd, &ud->stream);
        plumber_stream_init(pd, &ud->stream);
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

static runt_int plumb_stop(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    runt_int rc;

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);

    plumber_stop_jack(ud->pd, 0);
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

    plumber_stream_append_float(ud->pd, &ud->stream, s->f);

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
    plumber_stream_append_string(ud->pd, &ud->stream, str, strlen(str));
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

    rc = plumber_stream_append_ugen(ud->pd, &ud->stream, key);

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

    plumber_stream_destroy(ud->pd, stream);
    /* reinitialize */
    plumber_stream_init(ud->pd, stream);

    return RUNT_OK;
}

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

static runt_int plumb_new(runt_vm *vm, runt_ptr p)
{
/*
    runt_int rc;
    runt_stacklet *s;
    user_data *ud;

    runt_mark_set(vm);
    rc = runt_ppush(vm, &s);
    plumber_init(ud->pd);
    RUNT_ERROR_CHECK(rc);

    plumber_stream_init(ud->pd, &ud->stream);

    ud->pd->log = vm->fp;
*/
    return RUNT_OK; 
}

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

static runt_int plumb_parse(runt_vm *vm, runt_ptr p) 
{
    runt_int rc;
    runt_stacklet *s;
    const char *str;
    user_data *ud;
    plumber_data *pd;
    

    rc = plumb_data(vm, &ud);
    RUNT_ERROR_CHECK(rc);
    pd = ud->pd;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    str = runt_to_string(s->p);
  
    plumber_stream_parse_string(pd, &ud->stream, str);

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
    user_data *ud;
    plumber_data *pd;

    ud = (user_data *)runt_to_cptr(p);

    pd = ud->pd;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    varname = runt_to_string(s->p);

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    id = s->f;

    rc = runt_cell_pool_get_cell(vm, id + 1, &cell);
    RUNT_ERROR_CHECK(rc);
   
    var = (SPFLOAT *)cell->p.ud;

    runt_print(vm, "Creating sporth variable %s, length %d\n", 
            varname, strlen(varname));

    /*
    plumber_stream_append_data(pd, &ud->stream, varname, strlen(varname), var);
    */

    plumber_ftmap_delete(pd, 0);
    plumber_ftmap_add_userdata(pd, varname, var);
    plumber_ftmap_delete(pd, 1);
    return RUNT_OK;
}

static runt_int plumber_pd(runt_vm *vm, runt_ptr p)
{
    /* no need to do anything here, this is just used for connecting things */
    return RUNT_OK;
}

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

runt_int runt_load_plumber(runt_vm *vm)
{
    runt_print(vm, "loading plumber...\n");
    user_data *ud;
    runt_malloc(vm, sizeof(user_data), (void **)&ud);
    runt_ptr p = runt_mk_cptr(vm, ud);
    ud->magic = 123;

    plumb_define(vm, p, "plumb_new", 9, plumb_new);
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
    plumb_define(vm, p, "pd", 2, plumber_pd);
    return RUNT_OK;
}

typedef struct {
    runt_vm vm;
    unsigned char *buf;
    runt_cell *cells;
    runt_cell *create;
    runt_cell *init;
    runt_cell *compute;
    runt_cell *destroy;
} sporth_runt_data;

static void mk_runt(sporth_runt_data *rd)
{
    
    rd->buf = malloc(RUNT_MEGABYTE * 2);
    rd->cells = malloc(sizeof(runt_cell) * 1024);

    runt_cell_pool_set(&rd->vm, rd->cells, 1024);
    runt_cell_pool_init(&rd->vm);

    runt_memory_pool_set(&rd->vm, rd->buf, RUNT_MEGABYTE * 2);
    runt_init(&rd->vm);
}

static int load_runt(sporth_runt_data *rd, 
    const char *create, 
    const char *init,
    const char *compute,
    const char *destroy,
    const char *filename) 
{
    runt_vm *vm;
    runt_entry *tmp;
    runt_int rc;
    vm = &rd->vm;

    if(runt_parse_file(vm, filename) != RUNT_OK ) {
        return PLUMBER_NOTOK;
    }

    rc = runt_word_search(vm, create, strlen(create), &tmp);
    if(rc == RUNT_NOT_OK) {
        return PLUMBER_NOTOK;
    }

    rd->create = tmp->cell;
    
    rc = runt_word_search(vm, init, strlen(init), &tmp);
    if(rc == RUNT_NOT_OK) {
        return PLUMBER_NOTOK;
    }
    rd->init = tmp->cell;
    
    rc = runt_word_search(vm, compute, strlen(compute), &tmp);
    if(rc == RUNT_NOT_OK) {
        return PLUMBER_NOTOK;
    }
    rd->compute = tmp->cell;
    
    rc = runt_word_search(vm, destroy, strlen(destroy), &tmp);
    if(rc == RUNT_NOT_OK) {
        return PLUMBER_NOTOK;
    }
    rd->destroy = tmp->cell;

    return PLUMBER_OK;
}

int runt_plumber_create(plumber_data *pd, 
    sporth_stack *stack, 
    void **ud, 
    runt_int(* loader)(runt_vm *))
{
    sporth_runt_data *data;
    const char *create;
    const char *init;
    const char *compute;
    const char *destroy;
    const char *filename;
    runt_entry *ent;
    user_data *pud;

    data = malloc(sizeof(sporth_runt_data));
    *ud = data;
    mk_runt(data);

    filename = sporth_stack_pop_string(stack);
    destroy = sporth_stack_pop_string(stack);
    compute = sporth_stack_pop_string(stack);
    init = sporth_stack_pop_string(stack);
    create = sporth_stack_pop_string(stack);
    sporth_stack_pop_float(stack);

    loader(&data->vm);
    runt_word_search(&data->vm, "pd", 2, &ent);
    pud = (user_data *)runt_to_cptr(ent->cell->p);
    pud->pd = pd;

    if(load_runt(data, create, init, compute, destroy, filename) != PLUMBER_OK) {
        return PLUMBER_NOTOK;
    }

    runt_cell_exec(&data->vm, data->create);

    return PLUMBER_OK;
}

int runt_plumber_init(plumber_data *pd, sporth_stack *stack, 
    void **ud)
{
    sporth_runt_data *data;
    data = *ud;
    sporth_stack_pop_string(stack);
    sporth_stack_pop_string(stack);
    sporth_stack_pop_string(stack);
    sporth_stack_pop_string(stack);
    sporth_stack_pop_string(stack);
    sporth_stack_pop_float(stack);
    runt_cell_exec(&data->vm, data->init);
    return PLUMBER_OK;
}

int runt_plumber_compute(plumber_data *pd, sporth_stack *stack, void **ud)
{
    SPFLOAT in;
    sporth_runt_data *data;

    in = sporth_stack_pop_float(stack);
    data = *ud;
    if(in != 0) {
        runt_cell_exec(&data->vm, data->compute);
    }
    return PLUMBER_OK;
}

int runt_plumber_destroy(plumber_data *pd, sporth_stack *stack, void **ud)
{
    sporth_runt_data *data;

    data = *ud;
    runt_cell_exec(&data->vm, data->destroy);
    free(data->cells);
    free(data->buf);
    free(data);
    return PLUMBER_OK;
}
