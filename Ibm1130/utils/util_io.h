// util_io.h - definitions for simulator io routines

#ifndef FILE
#    include <stdio.h>
#endif

void util_io_init(void);
size_t fxread  (void *bptr, size_t size, size_t count, FILE *fptr);
size_t fxwrite (void *bptr, size_t size, size_t count, FILE *fptr);

/* Deal with MSVCRT renames: */
#if !defined(_WIN32)
#define util_fopen fopen
#else
static inline FILE *util_fopen(const char *fn, const char *mode)
{
    FILE *retval;

    if (fopen_s(&retval, fn, mode) != 0)
        retval = NULL;

    return retval;
}
#endif