#ifndef HELPER_H
#define HELPER_H

// #include <SDL3/SDL.h>

#define Struct(name) typedef struct name name; struct name
#define Enum(type, name) typedef type name; enum
#define Union(name) typedef union name name; union name

#define Bit_Toggle(var, bit) ((var) ^= (bit))
#define Bit_Set(var, bit)    ((var) |= (bit))
#define Bit_Clear(var, bit)  ((var) &= ~(bit))

// #define offsetof(_STRUCT, _MEMBER) ((size_t)((char *)&((_STRUCT *)0)->_MEMBER - (char *)0))

#endif // HELPER_H