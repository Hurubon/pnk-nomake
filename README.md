# pnk-nomake

A header-only C library for building C projects.

## 🚀 Getting started

#### Prerequisites

Put the nomake.h file somewhere in your project directory.

#### Build script

```c
#include <stdbool.h>

#define PNK_NOMAKE_BINARY_DIRECTORY "bin"
#define PNK_NOMAKE_SOURCE_DIRECTORY "."

#define PNK_NOMAKE_SOURCE   // STB-style single-header library
#include "lib/pnk/nomake.h" // Path to nomake.h

int main(int argc, char** argv)
{
    pnk_nomake_self_rebuild(argc, argv);

    pnk_nomake_project("pnk-nomake",
        .VERSION      = "0.3.0",
        .DESCRIPTION  = "No description.",
        .HOMEPAGE_URL = "https://github.com/Hurubon/pnk-nomake.git",
        .LANGUAGES    = "C");
    
    PnkNomakeTarget main = {
        .name    = "main",
        .sources = "src/main.c src/example.c",
        .include = "inc",
    };

    pnk_nomake_build_target(&main, .run = true);
}
```

#### Bootstrapping

```bash
cc -o nomake nomake.c
```

Now that nomake is bootstrapped, you never have to rebuild it again. To build your project, just run `nomake`.

## 📜 License

This software is released into the public domain.\
See [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- Nob library by tsoding on [YouTube](https://youtu.be/eRt7vhosgKE?si=ezTCFRf_g_SCQTVR), [GitHub](https://github.com/tsoding/nob.h)

## ✏️ Notes

`nomake` is still in active development. Contributions and suggestions are welcome and wanted!
