#ifndef BFX_COMPILE_H
#define BFX_COMPILE_H

#include "bfx.h"

#ifndef BFX_COMPILE_HEAD
#define BFX_COMPILE_HEAD "#include <stdio.h>\nint main(void) {unsigned char t[%ld];int p=0;"
#endif

#ifndef BFX_TMP_FILE_PATH
#define BFX_TMP_FILE_PATH "/tmp/bfx.c"
#endif

void bfx_compile(const char*, const char*, bfx_parameters_t);

#endif
