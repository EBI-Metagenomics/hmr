#ifndef DIE_H
#define DIE_H

#define die()                                                                  \
    do                                                                         \
    {                                                                          \
        __die(__FILE__, __LINE__);                                             \
    } while (0)

void __die(char const *file, int line) __attribute__((noreturn));

#endif
