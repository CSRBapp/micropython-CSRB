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
    /* Wall clock ceiling for a single execution, as a CLOCK_MONOTONIC
     * millisecond count; 0 means unlimited.  Armed by
     * mp_CSRB_execution_begin() and cleared by mp_CSRB_execution_end(). */
    uint64_t executionDeadlineMS;
    /* Set by the VM hook when the deadline above passes. */
    bool executionExpired;
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

/* Call before freeing the port context passed to mp_CSRB_init(): clears the
 * interpreter's reference to it.  Safe to call only after a matching init. */
extern void mp_CSRB_deinit(void);

/* Arm a wall clock ceiling on the execution that is about to start; a
 * timeoutMS of 0 leaves it unlimited.  Once the deadline passes, the VM hook
 * below raises KeyboardInterrupt in the running script. */
extern void mp_CSRB_execution_begin(const uint32_t timeoutMS);

/* Disarm the ceiling.  Returns true if the deadline was reached, which is what
 * distinguishes a timed out script from one that raised on its own. */
extern bool mp_CSRB_execution_end(void);

/* Deadline check, called from the VM's opcode loop through the
 * MICROPY_VM_HOOK_* macros in mpconfigport.h.  Not for direct use. */
extern "C" void mp_csrb_vm_hook(void);

#endif
