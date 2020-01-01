#include "py/runtime.h"
#include "py/stream.h"
#include "vfs_csrb.h"

#if MICROPY_VFS_CSRB

#include <unistd.h>
#include <stdint.h>

#include <CSRBvfs.h>

typedef struct _mp_obj_vfs_csrb_file_t {
    mp_obj_base_t base;
    mp_obj_t filename;
    uint64_t handle;
    uint32_t offset;
} mp_obj_vfs_csrb_file_t;

STATIC void check_fd_is_open(const mp_obj_vfs_csrb_file_t *o) {
    if (o->handle == 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "I/O operation on closed file"));
    }
}

STATIC void vfs_csrb_file_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_vfs_csrb_file_t *self = (mp_obj_vfs_csrb_file_t*)MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<io.%s %" PRIx64 ">", mp_obj_get_type_str(self_in), self->handle);
}

mp_obj_t mp_vfs_csrb_file_open(const mp_obj_type_t *type, mp_obj_t file_in, mp_obj_t mode_in) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);
    mp_obj_vfs_csrb_file_t *o = m_new_obj(mp_obj_vfs_csrb_file_t);

    const char *mode_s = mp_obj_str_get_str(mode_in);
    while (*mode_s) {
        switch (*mode_s++) {
            case 'b':
                type = &mp_type_vfs_csrb_fileio;
                break;
            case 't':
                type = &mp_type_vfs_csrb_textio;
                break;
        }
    }

    o->base.type = type;

    ret_t ret;
    uint64_t handle;
    handle = 0; /* a 0 handle is changed to a random number by the open() */

    mp_obj_t fid = file_in;
    const char *fname = mp_obj_str_get_str(fid);
    ret = port_ctx->csrbVFS->open(fname, handle);
    DEBUG(("open: %s ret:%" FORMAT_RET_T " handle:%" PRIx64 "\n", fname, ret, handle));
    switch(ret)
    {
        case RET_OK:
	    o->filename = file_in;
            o->handle = handle;
	    o->offset = 0;
            return MP_OBJ_FROM_PTR(o);
        default:
            o->handle = -1;
            return MP_OBJ_FROM_PTR(o);
    }
}

STATIC mp_obj_t vfs_csrb_file_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR_r)} },
    };

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, arg_vals);
    return mp_vfs_csrb_file_open(type, arg_vals[0].u_obj, arg_vals[1].u_obj);
}

STATIC mp_obj_t vfs_csrb_file_fileno(mp_obj_t self_in) {
    mp_obj_vfs_csrb_file_t *self = (mp_obj_vfs_csrb_file_t*)MP_OBJ_TO_PTR(self_in);
    check_fd_is_open(self);
    return MP_OBJ_NEW_SMALL_INT(self->handle);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(vfs_csrb_file_fileno_obj, vfs_csrb_file_fileno);

STATIC mp_obj_t vfs_csrb_file___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return mp_stream_close(args[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(vfs_csrb_file___exit___obj, 4, 4, vfs_csrb_file___exit__);

STATIC mp_uint_t vfs_csrb_file_read(mp_obj_t o_in, void *buf, mp_uint_t size, int *errcode) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);
    mp_obj_vfs_csrb_file_t *o = (mp_obj_vfs_csrb_file_t*)MP_OBJ_TO_PTR(o_in);
    check_fd_is_open(o);

    ret_t ret;
    uint32_t sizeRead;

    ret = port_ctx->csrbVFS->read(mp_obj_str_get_str(o->filename), o->handle, true, (char *)buf, size, o->offset, sizeRead);
    DEBUG(("read(): %s handle:%" PRIx64 " size:%lu sizeRead:%u ret:%" FORMAT_RET_T "\n",
        mp_obj_str_get_str(o->filename), o->handle, size, sizeRead, ret));
    switch(ret)
    {
       case RET_OK:
       case RET_NOTFOUND:      /* read all as 0s */
	   o->offset += ret;
           return sizeRead;
       case RET_EMPTY:
           return 0;
       case RET_TIMEOUT:
           *errcode = EAGAIN;
           break;
       case RET_FAIL:
           *errcode = ENOENT;
           break;
       default:
           *errcode = EIO;
           break;
    }

    return MP_STREAM_ERROR;
}

STATIC mp_uint_t vfs_csrb_file_write(mp_obj_t o_in, const void *buf, mp_uint_t size, int *errcode) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);
    mp_obj_vfs_csrb_file_t *o = (mp_obj_vfs_csrb_file_t*)MP_OBJ_TO_PTR(o_in);
    check_fd_is_open(o);

    ret_t ret;
    uint32_t sizeWritten;

    ret = port_ctx->csrbVFS->write(mp_obj_str_get_str(o->filename), o->handle, (char *)buf, size, o->offset, sizeWritten);
    DEBUG(("write(): %s handle:%" PRIx64 " size:%lu sizeWritten:%u ret:%" FORMAT_RET_T "\n",
        mp_obj_str_get_str(o->filename), o->handle, size, sizeWritten, ret));
    switch(ret)
    {
        case RET_OK:
            o->offset += sizeWritten;
            return sizeWritten;
        case RET_NOTFOUND:
        case RET_TIMEOUT:
            *errcode = EAGAIN;
            break;
        default:
            *errcode = EIO;
            break;
    }

    return MP_STREAM_ERROR;
}

STATIC mp_uint_t vfs_csrb_file_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mp_port_ctx_t *port_ctx = (mp_port_ctx_t*)MP_STATE(port_ctx);
    mp_obj_vfs_csrb_file_t *o = (mp_obj_vfs_csrb_file_t*)MP_OBJ_TO_PTR(o_in);
    check_fd_is_open(o);

    DEBUG(("ioctl(): %s handle:%" PRIx64 " request:%lu\n",
        mp_obj_str_get_str(o->filename), o->handle, request));

    switch (request) {
        case MP_STREAM_FLUSH:
            *errcode = 0;
            return 0;
        case MP_STREAM_SEEK: {
            struct mp_stream_seek_t *s = (struct mp_stream_seek_t*)arg;
            DEBUG(("ioctl(SEEK): %s handle:%" PRIx64 " whence:%d offset:%ld\n",
                mp_obj_str_get_str(o->filename), o->handle, s->whence, s->offset));
            switch(s->whence)
            {
                case SEEK_SET:
                    o->offset = s->offset;
                    break;
                case SEEK_CUR:
                    o->offset += s->offset;
                    break;
                case SEEK_END:
                default:
                    *errcode = EIO;
                    return MP_STREAM_ERROR;
            }
            *errcode = 0;
	    return 0;
        }
        case MP_STREAM_CLOSE:
            ret_t ret;
            ret = port_ctx->csrbVFS->close(mp_obj_str_get_str(o->filename), o->handle);
            if (ret != RET_OK)
            {
                *errcode = EIO;
                return MP_STREAM_ERROR;
            }
            *errcode = 0;
            return 0;
        default:
            *errcode = EINVAL;
            return MP_STREAM_ERROR;
    }
}

STATIC const mp_rom_map_elem_t rawfile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_fileno), MP_ROM_PTR(&vfs_csrb_file_fileno_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&vfs_csrb_file___exit___obj) },
};

STATIC MP_DEFINE_CONST_DICT(rawfile_locals_dict, rawfile_locals_dict_table);

STATIC const mp_stream_p_t fileio_stream_p = {
    .read = vfs_csrb_file_read,
    .write = vfs_csrb_file_write,
    .ioctl = vfs_csrb_file_ioctl,
};

const mp_obj_type_t mp_type_vfs_csrb_fileio = {
    { &mp_type_type },
    .name = MP_QSTR_FileIO,
    .print = vfs_csrb_file_print,
    .make_new = vfs_csrb_file_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &fileio_stream_p,
    .locals_dict = (mp_obj_dict_t*)&rawfile_locals_dict,
};

STATIC const mp_stream_p_t textio_stream_p = {
    .read = vfs_csrb_file_read,
    .write = vfs_csrb_file_write,
    .ioctl = vfs_csrb_file_ioctl,
    .is_text = true,
};

const mp_obj_type_t mp_type_vfs_csrb_textio = {
    { &mp_type_type },
    .name = MP_QSTR_TextIOWrapper,
    .print = vfs_csrb_file_print,
    .make_new = vfs_csrb_file_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &textio_stream_p,
    .locals_dict = (mp_obj_dict_t*)&rawfile_locals_dict,
};

#endif // MICROPY_VFS_CSRB
