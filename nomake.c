#include <stdio.h>

#define PNK_NOMAKE_IMPLEMENTATION
#include "lib/pnk/nomake.h"

int main(int argc, char** argv)
{
    PNK_NOMAKE_SELF_REBUILD(argc, argv);

    pnk_nomake_project("pnk-nomake",
        .VERSION      = "0.1.0",
        .DESCRIPTION  = "No description.",
        .HOMEPAGE_URL = "https://www.github.com/Hurubon/pnk-nomake");
    
    printf("NOMAKE_PROJECT_NAME: %s\n", PNK_NOMAKE_PROJECT_NAME);
    printf("NOMAKE_PROJECT_VERSION: %s\n", PNK_NOMAKE_PROJECT_VERSION);
    printf("NOMAKE_PROJECT_DESCRIPTION: %s\n", PNK_NOMAKE_PROJECT_DESCRIPTION);
    printf("NOMAKE_PROJECT_HOMEPAGE: %s\n", PNK_NOMAKE_PROJECT_HOMEPAGE);
    
    // PnkNomakeTarget main = pnk_nomake_add_executable("main.c");
}