#ifndef MICROPY_INCLUDED_EXTMOD_VFS_CSRB_H
#define MICROPY_INCLUDED_EXTMOD_VFS_CSRB_H

#include "py/lexer.h"
#include "py/obj.h"

extern const mp_obj_type_t mp_type_vfs_csrb;
extern const mp_obj_type_t mp_type_vfs_csrb_fileio;
extern const mp_obj_type_t mp_type_vfs_csrb_textio;

mp_obj_t mp_vfs_csrb_file_open(const mp_obj_type_t *type, mp_obj_t file_in, mp_obj_t mode_in);

#endif // MICROPY_INCLUDED_EXTMOD_VFS_CSRB_H
