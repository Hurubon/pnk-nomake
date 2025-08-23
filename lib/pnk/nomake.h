// Dedicated to the public domain under CC0 1.0.
// https://creativecommons.org/publicdomain/zero/1.0/
#ifndef PNK_NOMAKE_HEADER
#define PNK_NOMAKE_HEADER

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if  defined(__GNUC__)
    #define PNK_NOMAKE_C_COMPILER "gcc"
#elif defined(__clang__)
    #define PNK_NOMAKE_C_COMPILER "clang"
#else
    #error "Unknown compiler."
#endif

static char const* PNK_NOMAKE_PROJECT_NAME;
static char const* PNK_NOMAKE_PROJECT_VERSION;
static char const* PNK_NOMAKE_PROJECT_DESCRIPTION;
static char const* PNK_NOMAKE_PROJECT_HOMEPAGE;

typedef struct PnkNomakeTarget
{
    // TODO
} PnkNomakeTarget;

typedef struct PnkNomakeProjectInfo
{
    char const* VERSION;
    char const* DESCRIPTION;
    char const* HOMEPAGE_URL;
    // char const* LANGUAGES;
} PnkNomakeProjectInfo;

#define PNK_NOMAKE_SELF_REBUILD(argc, argv) \
    pnk_nomake_internal_self_rebuild(argc, argv, __FILE__)

#define PNK_NOMAKE_PRINT_LAST_ERROR() \
    pnk_nomake_internal_print_last_error(NULL, __FILE__, __LINE__, __func__)

#define PNK_NOMAKE_PRINT_LAST_ERROR_NOTE(note) \
    pnk_nomake_internal_print_last_error(note, __FILE__, __LINE__, __func__)

#define pnk_nomake_project(name, ...) \
    pnk_nomake_internal_project(name, &(PnkNomakeProjectInfo){ __VA_ARGS__ })

void pnk_nomake_internal_self_rebuild (int         argc,
                                       char**      argv,
                                       char const* source_file);

#endif /* PNK_NOMAKE_HEADER */

#if defined(PNK_NOMAKE_IMPLEMENTATION)

#ifdef _WIN32

    #include <windows.h>

    // TODO: Better error reporting?
    static inline
    void
    pnk_nomake_internal_print_last_error(
        char const* const restrict note,
        char const* const restrict file,
        int         const          line,
        char const* const restrict func)
    {
        // FIXME: static? global?
        char buffer[1024];
        DWORD dwError = GetLastError();
        DWORD dwSize  = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, dwError, 0, buffer, sizeof buffer, NULL);

        if (dwSize == 0)
        {

            printf("%s:%d: Unknown error in %s: %lu\n", file, line, func, dwError);
        }
        else if (note == NULL)
        {
            printf("%s:%d: error in %s: %s\n", file, line, func, buffer);
        }
        else
        {
            printf("%s:%d: error in %s: %s (%s)\n", file, line, func, buffer, note);
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
    enum PnkNomakeInternalNeedsRebuildResult {
        PNK_NOMAKE_NEEDS_REBUILD_ERROR_PATH_TOO_LONG,
        PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE,
        PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE,
        PNK_NOMAKE_NEEDS_REBUILD_NAY,
        PNK_NOMAKE_NEEDS_REBUILD_YAY,
    }
    pnk_nomake_internal_needs_rebuild(
        // FIXME: Can this be restrict?
        char const* const argv_zero,
        char const* const source_path)
    {        
        char binary_path[1024];
        strncpy(binary_path, argv_zero, sizeof binary_path);

        char const* const extension = argv_zero + strlen(argv_zero) - 4;
        if (strcmp(extension, ".exe") != 0)
        {
            strcpy(binary_path + strlen(argv_zero), ".exe");
        }

        time_t const binary_time = pnk_nomake_get_last_write_time(binary_path);
        time_t const source_time = pnk_nomake_get_last_write_time(source_path);
        if (binary_time == 0) return PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE;
        if (source_time == 0) return PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE;

        if (difftime(binary_time, source_time) < 0)
        {
            return PNK_NOMAKE_NEEDS_REBUILD_YAY;
        }
        else
        {
            return PNK_NOMAKE_NEEDS_REBUILD_NAY;
        }
    }

    void
    pnk_nomake_internal_self_rebuild(
        int         const          argc,
        char**      const restrict argv,
        char const* const restrict source_file)
    {
        switch (pnk_nomake_internal_needs_rebuild(argv[0], source_file))
        {
            case PNK_NOMAKE_NEEDS_REBUILD_ERROR_BINARY_FILE:
                PNK_NOMAKE_PRINT_LAST_ERROR_NOTE(argv[0]);
                exit(EXIT_FAILURE);
                break;
            case PNK_NOMAKE_NEEDS_REBUILD_ERROR_SOURCE_FILE:
                PNK_NOMAKE_PRINT_LAST_ERROR_NOTE(source_file);
                exit(EXIT_FAILURE);
                break;
            case PNK_NOMAKE_NEEDS_REBUILD_NAY:
                DeleteFile("nomake.old");
                return;
        }

        MoveFileEx("nomake.exe", "nomake.old", MOVEFILE_REPLACE_EXISTING);

        STARTUPINFO         siStartupInfo = { .cb = sizeof siStartupInfo, };
        PROCESS_INFORMATION piProcessInfo = { 0 };

        // TODO: Add PNK_NOMAKE_C_COMPILER customization point.
        if (!CreateProcessA(
            NULL, PNK_NOMAKE_C_COMPILER " nomake.c -o nomake.exe",
            NULL, NULL, TRUE, 0, NULL, NULL,
            &siStartupInfo, &piProcessInfo))
        {
            PNK_NOMAKE_PRINT_LAST_ERROR();
            exit(EXIT_FAILURE);
        }

        WaitForSingleObject(piProcessInfo.hProcess, INFINITE);

        CloseHandle(piProcessInfo.hProcess);
        CloseHandle(piProcessInfo.hThread);
        
        ZeroMemory(&piProcessInfo, sizeof piProcessInfo);
        ZeroMemory(&siStartupInfo, sizeof siStartupInfo);
        siStartupInfo.cb = sizeof siStartupInfo;

        if (!CreateProcessA(
            NULL, "nomake.exe",
            NULL, NULL, TRUE, 0, NULL, NULL,
            &siStartupInfo, &piProcessInfo))
        {
            PNK_NOMAKE_PRINT_LAST_ERROR_NOTE("nomake.exe");
            exit(EXIT_FAILURE);
        }

        CloseHandle(piProcessInfo.hProcess);
        CloseHandle(piProcessInfo.hThread);
        exit(EXIT_SUCCESS);
    }

#endif /* _WIN32 */

void
pnk_nomake_internal_project(
    char const*                 const restrict project_name,
    PnkNomakeProjectInfo const* const restrict project_info)
{
    PNK_NOMAKE_PROJECT_NAME = project_name;
    PNK_NOMAKE_PROJECT_VERSION     = project_info->VERSION;
    PNK_NOMAKE_PROJECT_DESCRIPTION = project_info->DESCRIPTION;
    PNK_NOMAKE_PROJECT_HOMEPAGE    = project_info->HOMEPAGE_URL;
}

#endif /* PNK_NOMAKE_IMPLEMENTATION */
