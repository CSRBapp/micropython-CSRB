#include <stdio.h>
#include <string.h>

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

void mp_CSRB_init(mp_port_ctx_t *port_ctx) {
    DEBUG(("mp_CSRB_init(): entry mp_module___main__=%p port_ctx=%p\n", &mp_module___main__, port_ctx));

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
    /* TODO: cleanup! */
}

