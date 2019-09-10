#ifndef MICROPY_INCLUDED_EXTMOD_VFS_CSRB_H
#define MICROPY_INCLUDED_EXTMOD_VFS_CSRB_H

#include "py/lexer.h"
#include "py/obj.h"
#include "extmod/vfs.h"

// these are the values for fs_user_mount_t.flags
#define FSUSER_NATIVE       (0x0001) // readblocks[2]/writeblocks[2] contain native func
#define FSUSER_FREE_OBJ     (0x0002) // fs_user_mount_t obj should be freed on umount
#define FSUSER_HAVE_IOCTL   (0x0004) // new protocol with ioctl
#define FSUSER_NO_FILESYSTEM (0x0008) // the block device has no filesystem on it

typedef struct _fs_user_mount_t {
#if 0
    mp_obj_base_t base;
    uint16_t flags;
    mp_obj_t readblocks[4];
    mp_obj_t writeblocks[4];
    // new protocol uses just ioctl, old uses sync (optional) and count
    union {
        mp_obj_t ioctl[4];
        struct {
            mp_obj_t sync[2];
            mp_obj_t count[2];
        } old;
    } u;
    CSRBFS csrbfs;
#endif
} fs_user_mount_t;

extern const byte fresult_to_errno_table[20];
extern const mp_obj_type_t mp_csrb_vfs_type;
extern const mp_obj_type_t mp_type_vfs_csrb_fileio;
extern const mp_obj_type_t mp_type_vfs_csrb_textio;

MP_DECLARE_CONST_FUN_OBJ_3(csrb_vfs_open_obj);

#endif // MICROPY_INCLUDED_EXTMOD_VFS_CSRB_H
