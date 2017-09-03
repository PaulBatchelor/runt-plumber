#define PLUMBER_STREAM_DATA SPORTH_LAST + 1

typedef struct plumber_word {
    uint32_t type;
    SPFLOAT flt;
    char *str;
    void *ud;
    struct plumber_word *next;
    /* only needed for ftable data */
    char deletable;
    char fttype;
} plumber_word;

typedef struct {
    plumber_word root;
    plumber_word *last;
    uint32_t size;
} plumber_stream;

/* word: a to-be plumber_pipe */
int plumber_word_alloc(plumber_data *pd, plumber_word **word);
int plumber_word_init(plumber_data *pd, plumber_word *word);
int plumber_word_free(plumber_data *pd, plumber_word *word);

/* stream: a to-be plumber_pipes */
int plumber_stream_init(plumber_data *pd, plumber_stream *stream);
int plumber_stream_append(plumber_data *pd, 
        plumber_stream *stream, plumber_word *word);
int plumber_stream_destroy(plumber_data *pd, plumber_stream *stream);

/* data types to append to the stream */
int plumber_stream_append_string(plumber_data *pd, 
        plumber_stream *stream, const char *str, uint32_t size);
int plumber_stream_append_ugen(plumber_data *pd, 
        plumber_stream *stream, const char *key);
int plumber_stream_append_float(plumber_data *pd, 
        plumber_stream *stream, SPFLOAT flt);
int plumber_stream_append_data(plumber_data *pd, 
        plumber_stream *stream, 
        const char *name, 
        uint32_t size, 
        void *ud,
        char deletable,
        char type);
int plumber_stream_append_function(plumber_data *pd, 
        plumber_stream *stream, const char *name, uint32_t size, 
        plumber_dyn_func f, void *ud);

/* get the function id of a ugen */
int plumber_str_to_ugen(plumber_data *pd, const char *str, uint32_t *id);

/* parse a string and add it to the stream */
int plumber_stream_parse_string(plumber_data *pd, 
        plumber_stream *stream, const char *str);

/* parses a stream and converts it to plumbing */
int plumbing_parse_stream(plumber_data *pd, 
        plumbing *pipes, 
        plumber_stream *stream);

/* this is the function that replaces plumber_reparse_string */
int plumber_reparse_stream(plumber_data *pd, plumber_stream *stream);

/* call this when you want to recompile */
int plumber_recompile_stream(plumber_data *pd, plumber_stream *str);

/* appends stream directly to plumbing pipes. */
int plumber_stream_append_to_main(plumber_data *pd, plumber_stream *stream);
