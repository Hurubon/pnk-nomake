#include <stdio.h>

#define PNK_NOMAKE_BINARY_DIRECTORY "bin"
#define PNK_NOMAKE_SOURCE_DIRECTORY "."

#define PNK_NOMAKE_SOURCE_STATIC
#include "inc/pnk/nomake.h"

int main(int argc, char** argv)
{
    pnk_nomake_self_rebuild(argc, argv);

    pnk_nomake_project("pnk-nomake",
        .VERSION      = "0.3.1",
        .DESCRIPTION  = "No description.",
        .HOMEPAGE_URL = "https://github.com/Hurubon/pnk-nomake.git");

    printf("NOMAKE_PROJECT_NAME: %s\n"       , PNK_NOMAKE_PROJECT_NAME);
    printf("NOMAKE_PROJECT_VERSION: %s\n"    , PNK_NOMAKE_PROJECT_VERSION);
    printf("NOMAKE_PROJECT_DESCRIPTION: %s\n", PNK_NOMAKE_PROJECT_DESCRIPTION);
    printf("NOMAKE_PROJECT_HOMEPAGE: %s\n"   , PNK_NOMAKE_PROJECT_HOMEPAGE);

    PnkNomakeTarget main = {
        .name    = "main",
        .sources = "src/main.c src/example.c",
        .include = "inc"
    };

    pnk_nomake_build_target(&main, .run = true);
}
