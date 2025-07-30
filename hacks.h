/* enums */

#define MY_TYPES     \
    X(UNKNOWN)    \
    X(FOO)        \
    X(COUNT)

typedef unsigned short My_Enum;
enum {
#define X(Type) Code_Type_##Type,
    MY_TYPES
#undef X
};

const char* Code_Type_Strings[] = {
#define X(Type) #Type,
    MY_TYPES
#undef X
};

/* dynamic arrays */

typedef struct My_Struct
{
    int a;
    float b;
} My_Struct;

#define ARRAY_TYPES \
    X(My_Struct)        \
    X(char)

#define X(Type) \
     struct Array_##Type Array_##Type;
ARRAY_TYPES
#undef X

#define X(Type)                 \
    typedef struct Array_##Type \
    {                           \
        Type *arr;      \
        Sint32 len;        \
        Sint32 cap;        \
    } Array_##Type;
ARRAY_TYPES
#undef X

#define append(ARRAY, ELEMENT)                                                           \
    do                                                                                   \
    {                                                                                    \
        if ((ARRAY).len == (ARRAY).cap)                                                  \
        {                                                                                \
            u32 new_cap = (ARRAY).cap ? 2 * (ARRAY).cap : 4096 / sizeof((ARRAY).arr[0]); \
            (ARRAY).arr = realloc((ARRAY).arr, new_cap * sizeof((ARRAY).arr[0]));        \
            (ARRAY).cap = new_cap;                                                       \
        }                                                                                \
        (ARRAY).arr[(ARRAY).len] = ELEMENT;                                              \
        (ARRAY).len += 1;                                                                \
    } while (0)

#define POP(ARRAY)          \
    do                      \
    {                       \
        if (ARRAY.len)      \
        {                   \
            ARRAY.len -= 1; \
        }                   \
    } while (0)

void append_string(char_Array* ca, bool null_term, char* s, u32 len)
{
    /*
        Note: if null_term is true, ca.arr will be a collection of
        null separated strings, not a single C string
    */
    if (ca->len + len + 1 > ca->cap)
    {
        u32 new_cap = ca->cap ? 1.5 * ca->cap : 4096 / sizeof(char);
        while (ca->len + len + 1 > new_cap)
        {
            new_cap = 1.5 * new_cap;
        }
        ca->arr = realloc(ca->arr, new_cap * sizeof(char));
        ca->cap = new_cap;
    }
    memcpy(ca->arr + ca->len, s, len);
    ca->len += len + null_term;
    ca->arr[ca->len - null_term] = '\0';
}

void append_fstring(char_Array* ca, bool null_term, char* format, ...)
{
    /*
        Note: if null_term is true, ca.arr will be a collection of
        null separated strings, not a single C string
    */
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);

    u32 len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (ca->len + len + 1 > ca->cap)
    {
        u32 new_cap = ca->cap ? 1.5 * ca->cap : 4096 / sizeof(char);
        while (ca->len + len + 1 > new_cap)
        {
            new_cap = 1.5 * new_cap;
        }
        ca->arr = realloc(ca->arr, new_cap * sizeof(char));
        ca->cap = new_cap;
    }

    va_start(args, format);
    vsnprintf(ca->arr + ca->len, len + 1, format, args);
    va_end(args);

    ca->len += len + null_term;
}
