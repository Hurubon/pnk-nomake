// Dedicated to the public domain under CC0 1.0.
// https://creativecommons.org/publicdomain/zero/1.0/
#ifndef PNK_NOMAKE_HEADER
#define PNK_NOMAKE_HEADER

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

#define pnk_nomake_project(name, ...) \
    pnk_nomake_internal_project(name, &(PnkNomakeProjectInfo){ __VA_ARGS__ })

void pnk_nomake_internal_self_rebuild (int         argc,
                                       char**      argv,
                                       char const* source_file);

#endif /* PNK_NOMAKE_HEADER */

#if defined(PNK_NOMAKE_IMPLEMENTATION)

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

#ifdef _WIN32

    #include <windows.h>

    // TODO: Better error reporting?
    static inline
    void
    pnk_nomake_print_last_error()
    {
        // FIXME: static? global?
        char buffer[1024];
        DWORD dwError = GetLastError();
        DWORD dwSize  = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, dwError, 0, buffer, sizeof buffer, NULL);

        if (dwSize == 0)
        {
            printf("Unknown error: %lu\n", dwError);
        }
        else
        {
            printf("Error: %s\n", buffer);
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
            ULARGE_INTEGER number = {
                .LowPart  = data.ftLastWriteTime.dwLowDateTime,
                .HighPart = data.ftLastWriteTime.dwHighDateTime,
            };

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
            pnk_nomake_print_last_error();
            exit(EXIT_FAILURE);
        }
    }

    static inline
    bool
    pnk_nomake_needs_rebuild(
        char const* const restrict binary_path,
        char const* const restrict source_path)
    {
        // FIXME: Could there be weird cases with files that
        time_t const binary_time = pnk_nomake_get_last_write_time(binary_path);
        time_t const source_time = pnk_nomake_get_last_write_time(source_path);
        if (difftime(binary_time, source_time) < 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void
    pnk_nomake_internal_self_rebuild(
        int         const          argc,
        char**      const restrict argv,
        char const* const restrict source_file)
    {
        // FIXME: Can we just pass argv[0] directly? Probably not.
        if (!pnk_nomake_needs_rebuild(argv[0], source_file))
        {
            return;
        }

        PROCESS_INFORMATION piProcessInfo;
        STARTUPINFO         siStartupInfo;
        ZeroMemory(&piProcessInfo, sizeof piProcessInfo);
        ZeroMemory(&siStartupInfo, sizeof siStartupInfo);

        BOOL bSuccess = CreateProcessA(
            NULL, "gcc nomake.c -o nomake.exe",
            NULL, NULL, TRUE, 0, NULL, NULL,
            &siStartupInfo, &piProcessInfo);

        if (!bSuccess)
        {
            pnk_nomake_print_last_error();
            exit(EXIT_FAILURE);
        }
        
        CloseHandle(piProcessInfo.hThread);
    }

#endif /* _WIN32 */

#endif /* PNK_NOMAKE_IMPLEMENTATION */
