#ifndef PNK_STRING_BUILDER_HEADER
#define PNK_STRING_BUILDER_HEADER

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct PnkStringBuilder {
    char*  buffer;
    size_t length;
    size_t capacity;
} PnkStringBuilder;

#ifndef PNK_STRING_BUILDER_SOURCE_STATIC
    bool      pnk_string_builder_acquire    (PnkStringBuilder* builder,
                                            ptrdiff_t         size);
    void      pnk_string_builder_release    (PnkStringBuilder* builder);
    void      pnk_string_builder_clear      (PnkStringBuilder* builder);
    bool      pnk_string_builder_append     (PnkStringBuilder* builder,
                                            char const*       string);
    ptrdiff_t pnk_string_builder_append_many(PnkStringBuilder* builder,
                                            ptrdiff_t         count,
                                            ...);
#endif

#endif/*PNK_STRING_BUILDER_HEADER*/

#if defined(PNK_STRING_BUILDER_SOURCE) ||   \
    defined(PNK_STRING_BUILDER_SOURCE_STATIC)

#ifdef PNK_STRING_BUILDER_SOURCE_STATIC
    #define PNK_LINKAGE static inline
#else
    #define PNK_LINKAGE extern
#endif

// NOTE: https://oeis.org/A029744
static ptrdiff_t const pnk_string_builder_sizes[] = {
    1         , 2         , 3         , 4         ,
    6         , 8         , 12        , 16        ,
    24        , 32        , 48        , 64        ,
    96        , 128       , 192       , 256       ,
    384       , 512       , 768       , 1024      ,
    1536      , 2048      , 3072      , 4096      ,
    6144      , 8192      , 12288     , 16384     ,
    24576     , 32768     , 49152     , 65536     ,
    98304     , 131072    , 196608    , 262144    ,
    393216    , 524288    , 786432    , 1048576   ,
    1572864   , 2097152   , 3145728   , 4194304   ,
    6291456   , 8388608   , 12582912  , 16777216  ,
    25165824  , 33554432  , 50331648  , 67108864  ,
    100663296 , 134217728 , 201326592 , 268435456 ,
    402653184 , 536870912 , 805306368 , 1073741824,
    1610612736, 2147483648, 3221225472, 4294967296,
};
static ptrdiff_t const pnk_string_builder_sizes_count =
    sizeof(pnk_string_builder_sizes) / sizeof(*pnk_string_builder_sizes);

static inline
ptrdiff_t
pnk_string_builder_round_up(
    ptrdiff_t const value)
{
    for (ptrdiff_t i = 0; i < pnk_string_builder_sizes_count; i += 1)
    {
        if (value <= pnk_string_builder_sizes[i])
        {
            return pnk_string_builder_sizes[i];
        }
    }

    return value;
}

PNK_LINKAGE
bool
pnk_string_builder_acquire(
    PnkStringBuilder* const builder,
    ptrdiff_t         const size)
{
    ptrdiff_t const rounded_size = pnk_string_builder_round_up(size);

    builder->buffer   = malloc(rounded_size);
    builder->length   = 0;
    builder->capacity = rounded_size;

    return builder->buffer != NULL;
}

PNK_LINKAGE
void
pnk_string_builder_release(
    PnkStringBuilder* const builder)
{
    free(builder->buffer);
    memset(builder, 0, sizeof *builder);
}

PNK_LINKAGE
void
pnk_string_builder_clear(
    PnkStringBuilder* const builder)
{
    builder->length = 0;
}

PNK_LINKAGE
bool
pnk_string_builder_append_sized(
    PnkStringBuilder* const builder,
    char const*       const string,
    ptrdiff_t         const length)
{
    ptrdiff_t const required = builder->length + length + 1;

    if (builder->capacity <= required)
    {
        ptrdiff_t const new_capacity = pnk_string_builder_round_up(required);
        char*     const new_buffer   = realloc(builder->buffer, new_capacity);
        
        if (new_buffer == NULL)
        {
            return false;
        }

        builder->buffer   = new_buffer;
        builder->capacity = new_capacity;
    }

    for (ptrdiff_t i = 0; i < length; i += 1)
    {
        builder->buffer[builder->length++] = string[i];
    }

    return true;
}

PNK_LINKAGE
bool
pnk_string_builder_append(
    PnkStringBuilder* const builder,
    char const*       const string)
{
    return pnk_string_builder_append_sized(builder, string, strlen(string));
}

PNK_LINKAGE
bool
pnk_string_builder_append_char(
    PnkStringBuilder* const builder,
    char unsigned     const character)
{
    ptrdiff_t const required = builder->length + 1;

    if (builder->capacity <= required)
    {
        ptrdiff_t const new_capacity = pnk_string_builder_round_up(required);
        char*     const new_buffer   = realloc(builder->buffer, new_capacity);

        if (new_buffer == NULL)
        {
            return false;
        }

        builder->buffer   = new_buffer;
        builder->capacity = new_capacity;
    }

    builder->buffer[builder->length++] = character;
    return true;
}

PNK_LINKAGE
ptrdiff_t
pnk_string_builder_append_many(
    PnkStringBuilder* const builder,
    ptrdiff_t         const count,
    ...)
{
    ptrdiff_t i = 0;

    va_list args;
    va_start(args, count);

    for (i = 0; i < count; i += 1)
    {
        if (!pnk_string_builder_append(builder, va_arg(args, char const*)))
            goto exit;
    }

exit:
    va_end(args);
    return i;
}

PNK_LINKAGE
ptrdiff_t
pnk_string_builder_format(
    PnkStringBuilder* const builder,
    char const*       const format,
    ...)
{
    ptrdiff_t i = 0;

    va_list args;
    va_start(args, format);

    for (char const* it = format; *it != '\0'; it += 1)
    {
        if (it[0] != '%')
        {
            if (pnk_string_builder_append_char(builder, it[0]))
                continue;
            else
                goto exit;
        }

        switch (it[1])
        {
            case 'c':
                // NOTE: Passing char unsigned to va_arg produces a warning.
                char unsigned const character = va_arg(args, int);
                if (!pnk_string_builder_append_char(builder, character))
                    goto exit;
                it += 1;
                i += 1;
                break;

            case 's':
                char const* const string = va_arg(args, char const*);
                if (!pnk_string_builder_append(builder, string))
                    goto exit;
                it += 1;
                i += 1;
                break;

            case 'd':
            case 'i':
                char buffer[32];
                ptrdiff_t const size = sprintf(buffer, "%i", va_arg(args, int));
                if (size <= 0)
                    goto exit;
                if (!pnk_string_builder_append_sized(builder, buffer, size))
                    goto exit;
                it += 1;
                i += 1;
                break;

            case '%':
                if (!pnk_string_builder_append_char(builder, '%'))
                    goto exit;
                it += 1;
                break;

            default:
                assert(false && "Format specifier not implemented yet.");
        }
    }
exit:
    va_end(args);
    return i;
}


PNK_LINKAGE
char*
pnk_string_builder_c_string(
    PnkStringBuilder* const builder)
{
    builder->buffer[builder->length] = 0;
    return builder->buffer;
}

#undef PNK_LINKAGE
#endif/*PNK_STRING_BUILDER_SOURCE*/
