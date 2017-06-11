#include <soundpipe.h>
#include <sporth.h>
#include <stdlib.h>
#include <string.h>
#include "stream.h"

int plumber_word_alloc(plumber_data *pd, plumber_word **word)
{
    plumber_word *tmp = malloc(sizeof(plumber_word));
    *word = tmp;
    return PLUMBER_OK;
}

int plumber_word_init(plumber_data *pd, plumber_word *word)
{
    /* type 0 is a float, apparently... */
    word->type = 0;
    word->next = NULL;
    word->flt = 0.0;
    word->str = NULL;
    word->deletable = 0;
    return PLUMBER_OK;
}

int plumber_word_free(plumber_data *pd, plumber_word *word)
{
    if(word->type == SPORTH_STRING || word->type == PLUMBER_STREAM_DATA) {
        free(word->str);
    }
    free(word);
    return PLUMBER_OK;
}

int plumber_stream_init(plumber_data *pd, plumber_stream *stream)
{
    stream->last = &stream->root;
    stream->size = 0;
    return PLUMBER_OK;
}

int plumber_stream_append(plumber_data *pd, 
        plumber_stream *stream, plumber_word *word)
{
    stream->last->next = word;
    stream->last = word;
    stream->size++;
    return PLUMBER_OK;
}

int plumber_stream_append_float(plumber_data *pd, 
        plumber_stream *stream, SPFLOAT flt)
{
    plumber_word *word;
    plumber_word_alloc(pd, &word);
    plumber_word_init(pd, word);
    word->type = SPORTH_FLOAT;
    word->flt = flt;
    plumber_stream_append(pd, stream, word);
    return PLUMBER_OK;
}

int plumber_stream_append_string(plumber_data *pd, 
        plumber_stream *stream, const char *str, uint32_t size)
{
    plumber_word *word;
    plumber_word_alloc(pd, &word);
    plumber_word_init(pd, word);
    word->type = SPORTH_STRING;
    word->str = malloc(size + 1);
    memset(word->str, 0, size + 1);
    strncpy(word->str, str, size);
    plumber_stream_append(pd, stream, word);
    return PLUMBER_OK;
}

int plumber_str_to_ugen(plumber_data *pd, const char *key, uint32_t *id)
{
    if(sporth_search(&pd->sporth.dict, key, id) != SPORTH_OK) {

        return PLUMBER_NOTOK;
    } 
    return PLUMBER_OK;
}

int plumber_stream_append_ugen(plumber_data *pd, 
        plumber_stream *stream, const char *key)
{
    uint32_t id = 0;
    uint32_t rc;
    plumber_word *word;

    rc = plumber_str_to_ugen(pd, key, &id);
    if(rc == PLUMBER_NOTOK) {
        return PLUMBER_NOTOK;
    }

    plumber_word_alloc(pd, &word);
    plumber_word_init(pd, word);
    /* offset is needed because otherwise '+' conflicts with 'string' */
    word->type = id + SPORTH_FOFFSET;
    plumber_stream_append(pd, stream, word);
    return PLUMBER_OK;
}

int plumber_stream_append_data(plumber_data *pd, 
        plumber_stream *stream, 
        const char *name, 
        uint32_t size, 
        void *ud, 
        char deletable)
{
    plumber_word *word;
    plumber_word_alloc(pd, &word);
    plumber_word_init(pd, word);
    word->type = PLUMBER_STREAM_DATA;
    word->ud = ud;
    word->str = malloc(size + 1);
    word->deletable = deletable;
    memset(word->str, 0, size + 1);
    strncpy(word->str, name, size);
    plumber_stream_append(pd, stream, word);
    return PLUMBER_OK;
}

int plumber_stream_append_function(plumber_data *pd, 
        plumber_stream *stream, const char *name, uint32_t size, 
        plumber_dyn_func f, void *ud)
{
    /* code adopted from plumber_ftmap_add_function() */

    sporth_fload_d *fd = malloc(sizeof(sporth_fload_d));
    fd->fun = f;
    fd->ud = ud;

    return plumber_stream_append_data(pd, stream, name, size, (void *)fd, 1);
}

int plumber_stream_destroy(plumber_data *pd, plumber_stream *stream)
{
    uint32_t i;
    plumber_word *entry = stream->root.next;
    plumber_word *next;
    for(i = 0; i < stream->size; i++) {
        next = entry->next; 
        plumber_word_free(pd, entry);
        entry = next;
    }
    return PLUMBER_OK;
}

int plumbing_parse_stream(plumber_data *pd, 
        plumbing *pipes, 
        plumber_stream *stream)
{
    uint32_t i;
    sporth_data *sporth = &pd->sporth;
    plumber_word *entry = stream->root.next;
    int rc;
    sporth_func *f;
    pd->mode = PLUMBER_CREATE;
    char *tmp;
    for(i = 0; i < stream->size; i++) {
        switch(entry->type) {
            case SPORTH_FLOAT:
                plumber_add_float(pd, pipes, entry->flt);
                sporth_stack_push_float(&sporth->stack, entry->flt);
                break;
            case SPORTH_STRING:
                tmp = plumber_add_string(pd, pipes, entry->str);
                sporth_stack_push_string(&sporth->stack, &tmp);
                break;
            case PLUMBER_STREAM_DATA:
                plumber_ftmap_delete(pd, entry->deletable);
                plumber_ftmap_add_userdata(pd, entry->str, entry->ud);
                plumber_ftmap_delete(pd, 1);
                break;
            default:
                f = &sporth->flist[entry->type - SPORTH_FOFFSET];
                rc = f->func(&sporth->stack, f->ud);
                if(rc == PLUMBER_NOTOK) {
                    return PLUMBER_NOTOK;
                }
                break;
        }
        entry = entry->next; 
    }
    return PLUMBER_OK;
}

int plumber_reparse_stream(plumber_data *pd, plumber_stream *stream)
{
    plumbing *pipes = pd->tmp;
    if(plumbing_parse_stream(pd, pipes, stream) == PLUMBER_OK) {
        pd->tmp = pipes;
    } else {
        pd->tmp = pipes;
        return PLUMBER_NOTOK;
    }
    return PLUMBER_OK;
}

int plumber_recompile_stream(plumber_data *pd, plumber_stream *stream)
{
    int error;
    /* might be needed... not sure? */
    pd->fp = NULL;
    plumber_reinit(pd);
    error = plumber_reparse_stream(pd, stream);
    plumber_swap(pd, error);
    return PLUMBER_OK;
}

static int cheap_string_append(plumber_data *pd, plumber_stream *stream, 
    char *str, int32_t len) {
    const char *key = str;
    return plumber_stream_append_string(pd, stream, key, len);
}

static int cheap_ugen_append(plumber_data *pd, plumber_stream *stream, 
    char *str) {
    const char *ugen = str;
    return plumber_stream_append_ugen(pd, stream, ugen);
}

int plumber_stream_parse_string(plumber_data *pd, 
        plumber_stream *stream, const char *str)
{
    char *out; 
    char *tmp;
    uint32_t pos = 0;
    uint32_t size = (uint32_t)strlen(str);
    uint32_t len = 0;
    SPFLOAT flt;

    while(pos < size) {
        out = sporth_tokenizer((char *)str, size, &pos);
        len = (uint32_t)strlen(out);
        switch(sporth_lexer(out, len)) {
            case SPORTH_FLOAT:
                flt = atof(out);
                plumber_stream_append_float(pd, stream, flt);
                break;
            case SPORTH_STRING:
                tmp = out;
                tmp[len - 1] = '\0';
                tmp++;
                cheap_string_append(pd, stream, tmp, strlen(tmp));
                break;
            case SPORTH_WORD:
                tmp = out;
                /* don't truncate the last character here like the string */
                tmp[len] = '\0';
                tmp++;
                cheap_string_append(pd, stream, tmp, strlen(tmp));
                break;
            default:
                cheap_ugen_append(pd, stream, out);
                break;
        }
        free(out);    
    }
    return PLUMBER_OK;
}

/* appends stream directly to plumbing pipes. */
int plumber_stream_append_to_main(plumber_data *pd, plumber_stream *stream)
{
    plumbing *pipes;
    pipes = plumber_get_pipes(pd);
    return plumbing_parse_stream(pd, pipes, stream);
}
