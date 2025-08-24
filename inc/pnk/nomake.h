// Dedicated to the public domain under CC0 1.0.
// https://creativecommons.org/publicdomain/zero/1.0/
#ifndef PNK_NOMAKE_HEADER
#define PNK_NOMAKE_HEADER

    #include <stdio.h>
    #include <stdbool.h>

    typedef struct PnkNomakeTarget {
        char const* name;
        char const* sources;
        char const* include;
    } PnkNomakeTarget;

    #include "def/header.include"

#endif/*PNK_NOMAKE_HEADER*/

#if defined(PNK_NOMAKE_SOURCE) || defined(PNK_NOMAKE_SOURCE_STATIC)

    #include <time.h>

    #define PNK_STRING_BUILDER_SOURCE_STATIC
    #include "string_builder.h"

    typedef struct PnkNomakeInternalProjectArgs {
        char const* VERSION;
        char const* DESCRIPTION;
        char const* HOMEPAGE_URL;
    } PnkNomakeInternalProjectArgs;

    typedef struct PnkNomakeInternalBuildTargetArgs {
        bool run;
    } PnkNomakeInternalBuildTargetArgs;

    typedef struct PnkNomakeInternalPrintLastErrorArgs {
        char const* note;
    } PnkNomakeInternalPrintLastErrorArgs;

    typedef struct PnkNomakeInternalSpawnProcessArgs {
        bool async;
    } PnkNomakeInternalSpawnProcessArgs;

    static char const* PNK_NOMAKE_PROJECT_NAME;
    static char const* PNK_NOMAKE_PROJECT_VERSION;
    static char const* PNK_NOMAKE_PROJECT_DESCRIPTION;
    static char const* PNK_NOMAKE_PROJECT_HOMEPAGE;

    #include "def/source.include"

    #define PNK_NOMAKE_ERROR_STRING_BUFFER_SIZE 512
    #ifdef _WIN32
        #include "nomake_win32.include"
    #else
        #include "nomake_posix.include"
    #endif

    static inline
    enum PnkNomakeInternalNeedsRebuildResult {
        PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE = -2,
        PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE = -1,
        PNK_NOMAKE_NEEDS_REBUILD_NAY = false,
        PNK_NOMAKE_NEEDS_REBUILD_YAY = true,
    }
    pnk_nomake_internal_needs_rebuild(
        char const* const restrict argv_zero,
        char const* const restrict source)
    {
        char binary[512];
        pnk_nomake_internal_file_append_exe(binary, sizeof binary, argv_zero);

        time_t const binary_time = pnk_nomake_internal_file_get_last_write_time(binary);
        time_t const source_time = pnk_nomake_internal_file_get_last_write_time(source);
        if (binary_time == 0) return PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE;
        if (source_time == 0) return PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE;

        return difftime(binary_time, source_time) < 0;
    }

    PNK_LINKAGE
    void
    pnk_nomake_detail_self_rebuild(
        int         const argc,
        char**      const argv,
        char const* const source_file)
    {
        char const* const nomake_old  = PNK_NOMAKE_BINARY_DIRECTORY"/nomake.old";
        char const* const binary_file = argv[0];

        if (!pnk_nomake_internal_file_directory_exists(PNK_NOMAKE_BINARY_DIRECTORY))
        {
            pnk_nomake_internal_file_directory_create(PNK_NOMAKE_BINARY_DIRECTORY);
        }

        switch (pnk_nomake_internal_needs_rebuild(binary_file, source_file))
        {
            case PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE:
                PNK_NOMAKE_INTERNAL_PRINT_LAST_ERROR(.note = binary_file);
                exit(EXIT_FAILURE);
            case PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE:
                PNK_NOMAKE_INTERNAL_PRINT_LAST_ERROR(.note = source_file);
                exit(EXIT_FAILURE);
            case PNK_NOMAKE_NEEDS_REBUILD_NAY:
                pnk_nomake_internal_file_delete(nomake_old);
                return;
        }

        PnkStringBuilder builder;
        if (!pnk_string_builder_acquire(&builder, 128))
        {
            PNK_NOMAKE_INTERNAL_PRINT_ERROR("Memory allocation failed.");
            goto exit_failure;
        }

        if (3 != pnk_string_builder_format(&builder, "%s -o %s %s",
            PNK_NOMAKE_C_COMPILER, binary_file, source_file))
        {
            PNK_NOMAKE_INTERNAL_PRINT_ERROR("Formatting failed.");
            goto exit_failure;
        }

        pnk_nomake_internal_file_rename(binary_file, nomake_old);

        char* const build_command = pnk_string_builder_c_string(&builder);
        PNK_NOMAKE_INTERNAL_SPAWN_PROCESS(build_command);
        PNK_NOMAKE_INTERNAL_SPAWN_PROCESS(argv[0], .async = true);

        pnk_string_builder_release(&builder);
        exit(EXIT_SUCCESS);

        exit_failure:
                pnk_string_builder_release(&builder);
                exit(EXIT_FAILURE);
    }

    PNK_LINKAGE
    void
    pnk_nomake_detail_project(
        char const*                         name,
        PnkNomakeInternalProjectArgs const* args)
    {
        PNK_NOMAKE_PROJECT_NAME = name;
        PNK_NOMAKE_PROJECT_VERSION     = args->VERSION;
        PNK_NOMAKE_PROJECT_DESCRIPTION = args->DESCRIPTION;
        PNK_NOMAKE_PROJECT_HOMEPAGE    = args->HOMEPAGE_URL;
    }

    PNK_LINKAGE
    void
    pnk_nomake_detail_build_target(
        PnkNomakeTarget const*           const target,
        PnkNomakeInternalBuildTargetArgs const args)
    {
        PnkStringBuilder builder;
        if (!pnk_string_builder_acquire(&builder, 128))
        {
            PNK_NOMAKE_INTERNAL_PRINT_ERROR("Memory allocation failed.");
            goto exit_failure;
        }

        // TODO: Factor out.
        #ifdef __GNUC__
            char const* const FORMAT
                = PNK_NOMAKE_C_COMPILER" -o "PNK_NOMAKE_BINARY_DIRECTORY"/%s %s";
            

            if (2 != pnk_string_builder_format(&builder, FORMAT,
                    target->name, target->sources))
            {
                PNK_NOMAKE_INTERNAL_PRINT_ERROR("Formatting failed.");
                goto exit_failure;
            }

            if (target->include != NULL)
            {
                if (1 != pnk_string_builder_format(&builder, " -I %s",
                        target->include))
                {
                    PNK_NOMAKE_INTERNAL_PRINT_ERROR("Formatting failed.");
                    goto exit_failure;
                }
            }
        #elif defined(_MSC_VER)
            char const* const FORMAT
                = PNK_NOMAKE_C_COMPILER" /Fe:"PNK_NOMAKE_BINARY_DIRECTORY"/%s %s";

            if (2 != pnk_string_builder_format(&builder, FORMAT,
                    target->name, target->sources))
            {
                PNK_NOMAKE_INTERNAL_PRINT_ERROR("Formatting failed.");
                goto exit_failure;
            }

            if (target->include != NULL)
            {
                if (1 != pnk_string_builder_format(&builder, " /I ",
                        target->include))
                {
                    PNK_NOMAKE_INTERNAL_PRINT_ERROR("Formatting failed.");
                    goto exit_failure;
                }
            }
        #else
            #error "Unsupported compiler."
        #endif

        char* const build_command = pnk_string_builder_c_string(&builder);
        PNK_NOMAKE_INTERNAL_SPAWN_PROCESS(build_command);

        if (args.run)
        {
            pnk_string_builder_clear(&builder);

            count = pnk_string_builder_append_many(&builder, 2,
                PNK_NOMAKE_BINARY_DIRECTORY"/", target->name);
            
            if (count != 2)
            {
                PNK_NOMAKE_INTERNAL_PRINT_ERROR("Formatting failed.");
                goto exit_failure;
            }

            char* const run_command = pnk_string_builder_c_string(&builder);
            PNK_NOMAKE_INTERNAL_SPAWN_PROCESS(run_command, .async = true);
        }
            
        exit_success:
            pnk_string_builder_release(&builder);
            exit(EXIT_SUCCESS);

        exit_failure:
            pnk_string_builder_release(&builder);
            exit(EXIT_FAILURE);
    }

#undef PNK_LINKAGE
#endif/*PNK_NOMAKE_SOURCE*/
