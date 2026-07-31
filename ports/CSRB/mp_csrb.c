#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "py/builtin.h"
#include "py/mpstate.h"
#include "py/pystack.h"
#include "extmod/vfs.h"

#include "mp_csrb.h"
#include "vfs_csrb.h"

extern "C" void nlr_jump_fail(void *val) {
    fprintf(stderr, "MICROPYTHON FATAL: uncaught NLR %p\n", val);
    exit(1);
}

/* Appended once when script output stops fitting the console buffer.  The
 * buffer is a remote author's only view of their script, so tell them output
 * was dropped rather than letting a clean-looking tail pass for the whole. */
static const char mp_csrb_truncation_marker[] = "\n<<< output truncated >>>\n";

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

    if(port_ctx->consoleTruncated) {
        return;
    }

    /* Room is measured with the marker already reserved, so the marker fits
     * whenever truncation is declared. */
    const uint32_t reserved = (uint32_t)sizeof(mp_csrb_truncation_marker); /* includes the NUL */
    const uint32_t room = port_ctx->consoleBufferSize - port_ctx->consoleBufferUsage - reserved;

    toWrite = std::min(strSize, room);

    memcpy(port_ctx->consoleBuffer + port_ctx->consoleBufferUsage, str, toWrite);

    port_ctx->consoleBufferUsage += toWrite;

    if(toWrite < strSize) {
        memcpy(port_ctx->consoleBuffer + port_ctx->consoleBufferUsage,
            mp_csrb_truncation_marker, reserved);
        port_ctx->consoleBufferUsage += reserved - 1; /* keep the NUL out of the count */
        port_ctx->consoleTruncated = true;
        return;
    }

    /* NUL must go after the appended data, not at its start */
    port_ctx->consoleBuffer[port_ctx->consoleBufferUsage] = 0;

    //fprintf(stdout, "%" PRIx64 ":%s\n", port_ctx->CSRBcontext, str);
}

extern void mp_init(void);

/* Arena for Python call frames (MICROPY_ENABLE_PYSTACK).  Static rather than
 * per context because the frame pointers live in mp_state_ctx, which is
 * process global - the same reason executions are serialised.  Exhausting it
 * raises RuntimeError in the script; 256KB is a few thousand frames deep,
 * far past what the C stack check would allow anyway. */
static mp_obj_t mp_csrb_pystack[256 * 1024 / sizeof(mp_obj_t)];

void mp_CSRB_init(mp_port_ctx_t *port_ctx, const CSRBvfs::vfsUID& accessUID) {
    DEBUG(("mp_CSRB_init(): entry mp_module___main__=%p port_ctx=%p uid=%u gid=%u\n",
        &mp_module___main__, port_ctx, accessUID.fields.uid, accessUID.fields.gid));

    /* Must precede mp_init(). */
    mp_pystack_init(mp_csrb_pystack, &mp_csrb_pystack[MP_ARRAY_SIZE(mp_csrb_pystack)]);

    /* Set before mp_init() and the mount below, both of which reach the VFS. */
    port_ctx->accessUID = accessUID;

    /* The context is malloc()ed rather than zeroed by the caller, so the
     * fields this port owns have to be set explicitly. */
    port_ctx->executionDeadlineMS = 0;
    port_ctx->executionExpired = false;
    port_ctx->openFiles = NULL;
    port_ctx->consoleTruncated = false;

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

void mp_CSRB_file_opened(CSRBvfs::vfsHandle *handle, const char *filename) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);
    mp_csrb_open_file_t *entry;

    if((port_ctx == NULL) || (handle == NULL)) {
        return;
    }

    entry = (mp_csrb_open_file_t*)malloc(sizeof(mp_csrb_open_file_t));
    if(entry == NULL) {
        /* Nothing useful to do about it here: the file is open either way, and
         * refusing to record it only means it is closed later than it should
         * be, when the whole interpreter goes away. */
        DEBUG(("failed to record open handle %p (%s)\n", handle, filename));
        return;
    }

    /* The name is copied because the one held by the file object lives in the
     * GC heap, which the collector may reuse the moment the script drops the
     * file - long before this list is walked. */
    entry->filename = strdup((filename != NULL) ? filename : "");
    entry->handle = handle;
    entry->next = port_ctx->openFiles;
    port_ctx->openFiles = entry;
}

void mp_CSRB_file_closed(CSRBvfs::vfsHandle *handle) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);
    mp_csrb_open_file_t **link;

    if((port_ctx == NULL) || (handle == NULL)) {
        return;
    }

    for(link = &port_ctx->openFiles; *link != NULL; link = &(*link)->next) {
        mp_csrb_open_file_t *entry = *link;

        if(entry->handle != handle) {
            continue;
        }

        *link = entry->next;
        free(entry->filename);
        free(entry);

        return;
    }
}

/* Close whatever the script left open.  Scripts are not obliged to close their
 * files, and a handle that is never closed keeps the entry locked for as long
 * as the node runs, so this is the point at which they have to go. */
static void mp_csrb_close_open_files(mp_port_ctx_t *port_ctx) {
    while(port_ctx->openFiles != NULL) {
        mp_csrb_open_file_t *entry = port_ctx->openFiles;
        ret_t ret;

        port_ctx->openFiles = entry->next;

        if(port_ctx->csrbVFS != NULL) {
            CSRBvfs::vfsHandle *handle = entry->handle;

            ret = port_ctx->csrbVFS->close(entry->filename, &handle, true, true, true);
            DEBUG(("closing handle %p (%s) the script left open: %" FORMAT_RET_T "\n",
                entry->handle, entry->filename, ret));
        }

        free(entry->filename);
        free(entry);
    }
}

void mp_CSRB_deinit(void) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);

    if(port_ctx != NULL) {
        mp_csrb_close_open_files(port_ctx);
    }

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

bool mp_CSRB_io_retry(void) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);

    if((port_ctx == NULL) || (port_ctx->executionDeadlineMS == 0)) {
        return false;
    }

    return mp_csrb_monotonicMS() < port_ctx->executionDeadlineMS;
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

