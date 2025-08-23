// Dedicated to the public domain under CC0 1.0.
// https://creativecommons.org/publicdomain/zero/1.0/
#ifndef PNK_NOMAKE_HEADER
#define PNK_NOMAKE_HEADER

    #include <stdbool.h>
/*----------------------------------------------------------------------------**

    Macros

**----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------**
        Constants
**----------------------------------------------------------------------------*/
    #ifndef PNK_NOMAKE_SOURCE_DIRECTORY
        #define PNK_NOMAKE_SOURCE_DIRECTORY "."
    #endif

    #ifndef PNK_NOMAKE_BINARY_DIRECTORY
        #define PNK_NOMAKE_BINARY_DIRECTORY "."
    #endif

    #ifndef PNK_NOMAKE_C_COMPILER
        #if   defined(__GNUC__)
            #define PNK_NOMAKE_C_COMPILER "gcc"
        #elif defined(__clang__)
            #define PNK_NOMAKE_C_COMPILER "clang"
        #elif defined(_MSC_VER)
            #define PNK_NOMAKE_C_COMPILER "cl"
        #else
            #error "Unknown compiler."
        #endif
    #endif
/*----------------------------------------------------------------------------**
        Functions
**----------------------------------------------------------------------------*/
    #define pnk_nomake_print_last_error(...)      \
        pnk_nomake_internal_print_last_error(     \
            __FILE__, __LINE__, __func__,         \
            (PnkNomakeInternalPrintLastErrorArgs) \
                { .dummy = 0, __VA_ARGS__ })

    #define pnk_nomake_spawn_process(cmd, ...)    \
        pnk_nomake_internal_spawn_process(        \
            cmd,                                  \
            (PnkNomakeInternalSpawnProcessArgs)   \
                { .dummy = 0, __VA_ARGS__ })



    #define pnk_nomake_self_rebuild(argc, argv)   \
        pnk_nomake_internal_self_rebuild(         \
            argc, argv, __FILE__)

    #define pnk_nomake_project(name, ...)         \
        pnk_nomake_internal_project(              \
            name,                                 \
            &(PnkNomakeInternalProjectArgs)       \
                { .dummy = 0, __VA_ARGS__ })
/*----------------------------------------------------------------------------**

    Type declarations

**----------------------------------------------------------------------------*/
    typedef struct PnkNomakeInternalPrintLastErrorArgs {
        char dummy;
        char const* note;
    } PnkNomakeInternalPrintLastErrorArgs;

    typedef struct PnkNomakeInternalSpawnProcessArgs {
        char dummy;
        bool async;
    } PnkNomakeInternalSpawnProcessArgs;

    typedef struct PnkNomakeInternalProjectArgs {
        char        dummy;
        char const* VERSION;
        char const* DESCRIPTION;
        char const* HOMEPAGE_URL;
    } PnkNomakeInternalProjectArgs;

#endif /* PNK_NOMAKE_HEADER */

#if defined(PNK_NOMAKE_SOURCE)

    #include <stdio.h>
    #include <stdlib.h>
    #include <stddef.h>

    #include <time.h>
    #include <assert.h>
    #include <string.h>

    static char const* PNK_NOMAKE_PROJECT_NAME;
    static char const* PNK_NOMAKE_PROJECT_VERSION;
    static char const* PNK_NOMAKE_PROJECT_DESCRIPTION;
    static char const* PNK_NOMAKE_PROJECT_HOMEPAGE;

    #ifdef _WIN32
        
        #include <windows.h>

        static inline
        void
        pnk_nomake_internal_print_last_error(
            char const*                         const restrict file,
            int                                 const          line,
            char const*                         const restrict func,
            PnkNomakeInternalPrintLastErrorArgs const          args)
        {
            char buffer[512];
            DWORD dwError = GetLastError();
            DWORD dwSize  = FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dwError, 0, buffer, sizeof buffer, NULL);
            
                if (dwSize == 0)
                    printf("%s:%d: unknown error in %s: %lu\n",
                        file, line, func, dwError);
                else if (args.note == NULL)
                    printf("%s:%d: error in %s: %s\n",
                        file, line, func, buffer);
                else
                    printf("%s:%d: error in %s: %s (note: %s)\n",
                        file, line, func, buffer, args.note);
        }

        static inline
        void
        pnk_nomake_append_exe(
            char*       const restrict buffer,
            size_t      const          buffer_size,
            char const* const restrict path)
        {
            size_t      const path_size = strlen(path);
            char const* const extension = path + path_size - 4;

            assert(path_size + 4 < buffer_size && "Insufficient buffer size.");

            strncpy(buffer, path, buffer_size);

            if (strcmp(extension, ".exe") != 0)
            {
                strcpy(buffer + path_size, ".exe");
            }
        }

        static inline
        time_t
        pnk_nomake_get_last_write_time(
            char const* const path)
        {
            WIN32_FILE_ATTRIBUTE_DATA data;
            if (GetFileAttributesExA(path, GetFileExInfoStandard, &data))
            {
                ULARGE_INTEGER number;
                number.LowPart  = data.ftLastWriteTime.dwLowDateTime;
                number.HighPart = data.ftLastWriteTime.dwHighDateTime;

                // The number of seconds between
                //      UTC:1601/01/01 (Windows epoch) and
                //      UTC:1970/01/01 (Unix epoch).
                long long unsigned const epoch_difference = 11644473600ULL;
                // Windows counts in increments of 100ns, not seconds.
                long long unsigned const seconds = number.QuadPart / 10000000ULL;

                return seconds - epoch_difference;
            }
            else
            {
                return 0;
            }
        }

        static inline
        bool
        pnk_nomake_delete_file(
            char const* const path)
        {
            return DeleteFile(path);
        }

        static inline
        bool
        pnk_nomake_rename(
            char const* const restrict old_name,
            char const* const restrict new_name)
        {
            // NOTE: To match the behaviour on POSIX.
            return MoveFileEx(old_name, new_name, MOVEFILE_REPLACE_EXISTING);
        }

        static inline
        void
        pnk_nomake_internal_spawn_process(
            char*                             const cmd,
            PnkNomakeInternalSpawnProcessArgs const args)
        {
            STARTUPINFO         siStartupInfo = { .cb = sizeof siStartupInfo };
            PROCESS_INFORMATION piProcessInfo = { 0 };

            if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL,
                    &siStartupInfo, &piProcessInfo))
            {
                pnk_nomake_print_last_error();
                exit(EXIT_FAILURE);
            }

            if (args.async == false)
            {
                WaitForSingleObject(piProcessInfo.hProcess, INFINITE);
            }

            CloseHandle(piProcessInfo.hProcess);
            CloseHandle(piProcessInfo.hThread);
        }

    #else

        #include <unistd.h>
        #include <sys/stat.h>

        static inline
        void
        pnk_nomake_internal_print_last_error(
            char const*                         const restrict file,
            int                                 const          line,
            char const*                         const restrict func,
            PnkNomakeInternalPrintLastErrorArgs const          args)
        {
            char buffer[512];
            assert(strerror_r(errno, buffer, sizeof buffer) == 0 &&
                "strerror_r failed - insufficient buffer size?");

            if (note == NULL)
            {
                printf("%s:%d: error in %s: %s\n",
                    file, line, func, buffer);
            }
            else
            {
                printf("%s:%d: error in %s: %s (note: %s)\n",
                    file, line, func, buffer, note);
            }
        }

        static inline
        void
        pnk_nomake_append_exe(
            char*       const restrict buffer,
            size_t      const          buffer_size,
            char const* const restrict path)
        {
            size_t const path_size = strlen(path);

            assert(path_size < buffer_size && "Insufficient buffer size.");

            strncpy(buffer, path, buffer_size);
        }

        static inline
        time_t
        pnk_nomake_get_last_write_time(
            char const* const path)
        {
            struct stat data;

            if (stat(path, &data))
                return data.st_mtime;
            else
                return 0;
        }

        static inline
        bool
        pnk_nomake_delete_file(
            char const* const path)
        {
            return !unlink(path);
        }

        static inline
        bool
        pnk_nomake_rename(
            char const* const restrict old_name,
            char const* const restrict new_name)
        {
            return !rename(old_name, new_name);
        }

        static inline
        void
        pnk_nomake_internal_spawn_process(
            char const*                       const cmd,
            PnkNomakeInternalSpawnProcessArgs const args)
        {
            assert(false && "Not implemented yet.");
        }

    #endif /* _WIN32 */

    static inline
    enum PnkNomakeInternalNeedsRebuildResult {
        PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE = -2,
        PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE = -1,
        PNK_NOMAKE_NEEDS_REBUILD_NAY = false,
        PNK_NOMAKE_NEEDS_REBUILD_YAY = true,
    }
    pnk_nomake_needs_rebuild(
        char const* const restrict argv_zero,
        char const* const restrict source_path)
    {
        char binary_path[512];
        pnk_nomake_append_exe(binary_path, sizeof binary_path, argv_zero);

        time_t const binary_time = pnk_nomake_get_last_write_time(binary_path);
        time_t const source_time = pnk_nomake_get_last_write_time(source_path);
        if (binary_time == 0) return PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE;
        if (source_time == 0) return PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE;

        return difftime(binary_time, source_time) < 0;
    }

    static inline
    void
    pnk_nomake_internal_self_rebuild(
        int         const          argc,
        char**      const restrict argv,
        char const* const restrict source_file)
    {
        switch (pnk_nomake_needs_rebuild(argv[0], source_file))
        {
            case PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE:
                pnk_nomake_print_last_error(.note = argv[0]);
                exit(EXIT_FAILURE);
            case PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE:
                pnk_nomake_print_last_error(.note = source_file);
                exit(EXIT_FAILURE);
            case PNK_NOMAKE_NEEDS_REBUILD_NAY:
                pnk_nomake_delete_file(PNK_NOMAKE_BINARY_DIRECTORY"/nomake.old");
                return;
        }

        pnk_nomake_rename(argv[0], PNK_NOMAKE_BINARY_DIRECTORY"/nomake.old");
        pnk_nomake_spawn_process(PNK_NOMAKE_C_COMPILER" -o nomake nomake.c");
        pnk_nomake_spawn_process("nomake", .async = true);
        exit(EXIT_SUCCESS);
    }

    static inline
    void
    pnk_nomake_internal_project(
        char const*                         const restrict project_name,
        PnkNomakeInternalProjectArgs const* const restrict project_args)      
    {
        PNK_NOMAKE_PROJECT_NAME = project_name;
        PNK_NOMAKE_PROJECT_VERSION     = project_args->VERSION;
        PNK_NOMAKE_PROJECT_DESCRIPTION = project_args->DESCRIPTION;
        PNK_NOMAKE_PROJECT_HOMEPAGE    = project_args->HOMEPAGE_URL;
    }

#undef PNK_LINKAGE
#endif /* PNK_NOMAKE_SOURCE */