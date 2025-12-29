#pragma once

#include "sys_headers.h"
#include "library.h"

#if defined (CLI_ONLY)

    #include "cli.h"

#else

    #include "gui.h"

#endif

int exec(int argc, char *argv[], string libPath);