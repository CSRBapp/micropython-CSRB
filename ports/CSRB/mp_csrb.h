#ifndef _MP_CSRB_H_
#define _MP_CSRB_H_

#include <stdint.h>
#include <CSRBvfs.h>

#define __DEBUG(format, ...) do{ \
    fprintf(stderr, "[%16.16" PRIx64 "] [MP_CSRB_VFS] %s|%s:%u|# " format, \
        (uint64_t) pthread_self(), __FILE__,  __FUNCTION__, __LINE__, ## __VA_ARGS__); \
    }while(0)
#define DEBUG(x) __DEBUG x

typedef struct {
    CSRBvfs *csrbVFS;
    uint64_t csrbContext;
    char *consoleBuffer;
    uint32_t consoleBufferSize;
    uint32_t consoleBufferUsage;
} mp_port_ctx_t;

extern "C" void mp_csrb_print_strn(const char *str, const uint32_t strSize);
#define MP_PLAT_PRINT_STRN(str, len) mp_csrb_print_strn(str, len)

extern void mp_CSRB_init(mp_port_ctx_t *port_ctx);

#endif
