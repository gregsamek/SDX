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

/* strings */

//////////////////////////////////////////////////
Array_Char out = {0};
Array_Initialize(&out, 16384);
memset(out.arr, 0, out.cap);

#define Append_fmt(fmt, ...) \
    do { \
        int needed = snprintf(NULL, 0, fmt, ##__VA_ARGS__); \
        if (needed < 0) \
        { \
            fprintf(stderr, "snprintf encoding error!\n"); \
            break; \
        } \
        while (out.len + needed + 1 > out.cap) \
        { \
            int new_cap = out.cap * 2; \
            if (new_cap < out.len + needed + 1) { \
                new_cap = out.len + needed + 1; \
            } \
            void* temp_ptr = realloc(out.arr, new_cap); \
            if (temp_ptr == NULL) \
            { \
                fprintf(stderr, "Failed to reallocate array memory!\n"); \
                break;\
            } \
            out.arr = temp_ptr; \
            out.cap = new_cap; \
        } \
        int written = snprintf(out.arr + out.len, out.cap - out.len, fmt, ##__VA_ARGS__); \
        if (written > 0) out.len += written; \
    } while (0) 

Append_fmt("%s", header_guard);
Append_fmt("%s", cpp_compatibility);
//////////////////////////////////////////////////

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

/* defer */

#define macro_var(name) concat(name, __LINE__)
#define defer(start, end) \
    for (int macro_var(_i_) = ((start), 0); !macro_var(_i_); ++macro_var(_i_), (end))

