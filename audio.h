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

int start_audio(plumber_data *pd, 
        void *ud, void (*callback)(sp_data *, void *), int port);
int stop_audio(sp_jack *jd);
