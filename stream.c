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
    return PLUMBER_OK;
}

int plumber_word_free(plumber_data *pd, plumber_word *word)
{
    if(word->type == SPORTH_STRING) free(word->str);
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
    word->type = id;
    plumber_stream_append(pd, stream, word);
    return PLUMBER_OK;
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
            default:
                f = &sporth->flist[entry->type];
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
