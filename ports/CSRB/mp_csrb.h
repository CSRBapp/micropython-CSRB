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
    /* Identity every VFS operation runs as.  Supplied by the caller through
     * mp_CSRB_init(); CSRBfs stores it as the uid/gid of anything a script
     * creates, so a default-constructed (0/0) value makes scripts run with
     * the privileged identity CSRBvfsFUSE reserves for bootstrapping. */
    CSRBvfs::vfsUID accessUID;
    char *consoleBuffer;
    uint32_t consoleBufferSize;
    uint32_t consoleBufferUsage;
} mp_port_ctx_t;

extern "C" void mp_csrb_print_strn(const char *str, const uint32_t strSize);
#define MP_PLAT_PRINT_STRN(str, len) mp_csrb_print_strn(str, len)

/* accessUID is a required argument rather than another field for the caller to
 * fill in, because the port context is malloc()ed without being zeroed: an
 * unset field would be an arbitrary uid/gid rather than a merely wrong one. */
extern void mp_CSRB_init(mp_port_ctx_t *port_ctx, const CSRBvfs::vfsUID& accessUID);

#endif
