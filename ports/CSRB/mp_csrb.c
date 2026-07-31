#include <stdio.h>
#include <string.h>
#include <time.h>

#include "py/builtin.h"
#include "py/mpstate.h"
#include "extmod/vfs.h"

#include "mp_csrb.h"
#include "vfs_csrb.h"

extern "C" void nlr_jump_fail(void *val) {
    fprintf(stderr, "MICROPYTHON FATAL: uncaught NLR %p\n", val);
    exit(1);
}

void mp_csrb_print_strn(const char *str, const uint32_t strSize) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);

#if 0
    fprintf(stdout, "[%" PRIx64 "] consoleBuffer:%p, consoleBufferSize:%u\n",
        port_ctx->CSRBcontext,
        port_ctx->consoleBuffer,
        port_ctx->consoleBufferSize);

    fprintf(stdout, "[%" PRIx64 "] APPENDING [%s] to [%s]\n",
        port_ctx->CSRBcontext, str, port_ctx->consoleBuffer);
#endif
    uint32_t toWrite;

    toWrite = std::min(strSize, port_ctx->consoleBufferSize - port_ctx->consoleBufferUsage - 1);

    memcpy(port_ctx->consoleBuffer + port_ctx->consoleBufferUsage, str, toWrite);

    port_ctx->consoleBufferUsage += toWrite;

    /* NUL must go after the appended data, not at its start */
    port_ctx->consoleBuffer[port_ctx->consoleBufferUsage] = 0;

    //fprintf(stdout, "%" PRIx64 ":%s\n", port_ctx->CSRBcontext, str);
}

extern void mp_init(void);

void mp_CSRB_init(mp_port_ctx_t *port_ctx, const CSRBvfs::vfsUID& accessUID) {
    DEBUG(("mp_CSRB_init(): entry mp_module___main__=%p port_ctx=%p uid=%u gid=%u\n",
        &mp_module___main__, port_ctx, accessUID.fields.uid, accessUID.fields.gid));

    /* Set before mp_init() and the mount below, both of which reach the VFS. */
    port_ctx->accessUID = accessUID;

    /* The context is malloc()ed rather than zeroed by the caller, so the
     * deadline this port owns has to be disarmed explicitly. */
    port_ctx->executionDeadlineMS = 0;
    port_ctx->executionExpired = false;

    MP_STATE(port_ctx) = port_ctx;

    mp_init();

    {
    // Mount the CSRB FS at the root of our internal VFS
        mp_obj_t args[2] = {
            mp_type_vfs_csrb.make_new(&mp_type_vfs_csrb, 0, 0, NULL),
            MP_OBJ_NEW_QSTR(MP_QSTR__slash_),
        };
        mp_vfs_mount(2, args, (mp_map_t*)&mp_const_empty_map);
        MP_STATE_VM(vfs_cur) = MP_STATE_VM(vfs_mount_table);
        DEBUG(("mounted CSRB VFS %p %p\n", MP_STATE_VM(vfs_cur), MP_STATE_VM(vfs_mount_table)));
    }
}

void mp_CSRB_deinit(void) {
    /* mp_state_ctx is process global and outlives the caller's port context.
     * Drop the reference so nothing reached afterwards - mp_csrb_print_strn()
     * in particular - writes through a consoleBuffer the caller has freed. */
    MP_STATE(port_ctx) = NULL;
}

static uint64_t mp_csrb_monotonicMS(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((uint64_t) ts.tv_sec * 1000u) + ((uint64_t) ts.tv_nsec / 1000000u);
}

void mp_CSRB_execution_begin(const uint32_t timeoutMS) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);

    if(port_ctx == NULL) {
        return;
    }

    port_ctx->executionDeadlineMS = (timeoutMS != 0)
        ? mp_csrb_monotonicMS() + timeoutMS
        : 0;
    port_ctx->executionExpired = false;
}

bool mp_CSRB_execution_end(void) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);

    if(port_ctx == NULL) {
        return false;
    }

    /* Disarm before returning: the deadline belongs to the execution that just
     * finished, and anything run afterwards - the exception printer, a later
     * call - must not inherit an already expired one. */
    port_ctx->executionDeadlineMS = 0;

    /* The VM can return - through MICROPY_VM_HOOK_RETURN, or by leaving the
     * loop before the pending exception check - with an interrupt the hook
     * planted still armed.  Withdraw it, or it fires during whatever runs
     * next, quite possibly with no handler pushed. */
    if(MP_STATE_VM(mp_pending_exception) == MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception))) {
        MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
    }

    return port_ctx->executionExpired;
}

void mp_csrb_vm_hook(void) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);

    if((port_ctx == NULL) || (port_ctx->executionDeadlineMS == 0)) {
        return;
    }

    if(mp_csrb_monotonicMS() < port_ctx->executionDeadlineMS) {
        return;
    }

    port_ctx->executionExpired = true;

    /* Re-raise on every hook rather than only on the first one: a script that
     * swallows the interrupt with a bare "except" would otherwise carry on
     * running, and the caller serialises executions behind a mutex, so a
     * script that never returns blocks every later one.  Re-arming means such
     * a script is interrupted again at the next backwards jump and makes no
     * further progress.
     *
     * The exception object is the one mp_init() preallocated, so raising it
     * here costs no allocation.  Note that the VM only reaches this hook
     * between bytecodes: a single long running runtime call cannot be cut
     * short. */
    if(MP_STATE_VM(mp_pending_exception) == MP_OBJ_NULL) {
        MP_STATE_VM(mp_pending_exception) = MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
    }
}

