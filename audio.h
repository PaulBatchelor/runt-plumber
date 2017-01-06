typedef struct {
    sp_data *sp;
    plumber_data *pd;
    jack_port_t **output_port;
    jack_port_t *input_port;
    jack_client_t **client;
    SPFLOAT in;
    void *ud;
    void (*callback)(sp_data *, void *);
    char run;
    const char **ports;
} sp_jack;

typedef struct {
    plumber_data pd;
    sp_data *sp;
    sp_jack jd;
} user_data;

int start_audio(plumber_data *pd, 
        void *ud, void (*callback)(sp_data *, void *), int port);
int stop_audio(user_data *ud);
