extern "C" void nlr_jump_fail(void *val) {
    //printf("FATAL: uncaught NLR %p\n", val);
    //exit(1);
}

#if !MICROPY_VFS
#include <py/lexer.h>
#include <sys/stat.h>
mp_import_stat_t mp_import_stat(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return MP_IMPORT_STAT_DIR;
        } else if (S_ISREG(st.st_mode)) {
            return MP_IMPORT_STAT_FILE;
        }
    }
    return MP_IMPORT_STAT_NO_EXIST;
}
#endif

void mp_reader_new_file_from_fd(mp_reader_t *reader, int fd, bool close_fd) {
#if 0
    mp_reader_posix_t *rp = m_new_obj(mp_reader_posix_t);
    rp->close_fd = close_fd;
    rp->fd = fd;
    int n = read(rp->fd, rp->buf, sizeof(rp->buf));
    if (n == -1) {
        if (close_fd) {
            close(fd);
        }
        mp_raise_OSError(errno);
    }
    rp->len = n;
    rp->pos = 0;
    reader->data = rp;
    reader->readbyte = mp_reader_posix_readbyte;
    reader->close = mp_reader_posix_close;
#endif
}

