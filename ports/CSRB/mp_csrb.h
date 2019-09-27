#ifndef _MP_CSRB_H_
#define _MP_CSRB_H_

#include <stdint.h>
#include <CSRBvfs.h>

#define __DEBUG(format, ...) do{ fprintf(stderr, "[MP_CSRB_VFS] %s:%u| " format, __FUNCTION__, __LINE__, ## __VA_ARGS__); }while(0)
#define DEBUG(x) __DEBUG x

typedef struct {
    CSRBvfs *csrbVFS;
    uint64_t csrbContext;
    char *stdout;
    uint32_t stdoutSize;
    uint32_t stdoutUsage;
} mp_port_ctx_t;

extern "C" void mp_csrb_print_strn(const char *str, const uint32_t strSize);
#define MP_PLAT_PRINT_STRN(str, len) mp_csrb_print_strn(str, len)

extern void mp_CSRB_init(mp_port_ctx_t *port_ctx);

#endif
