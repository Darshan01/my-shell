//arraylist of characters
typedef struct {
    char* data;
    int pos;
    int length;
    int capacity;
} alCH_t;

void alch_init(alCH_t *, int);
void alch_destroy(alCH_t *);

void alch_push(alCH_t *, char);
void alch_pushStr(alCH_t *, char*);

void alch_resize(alCH_t *);
void alch_move(alCH_t *);