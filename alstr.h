//arraylist of strings
typedef struct {
    char** data;
    unsigned length;
    unsigned capacity;
} alSTR_t;

void alstr_init(alSTR_t *, unsigned);
void alstr_destroy(alSTR_t *);

void alstr_push(alSTR_t *, char*);
