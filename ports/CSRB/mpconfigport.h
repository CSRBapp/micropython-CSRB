#define MICROPY_ALLOC_PATH_MAX      (PATH_MAX)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_ENABLE_FINALISER    (0)
#define MICROPY_STACK_CHECK         (0)
#define MICROPY_COMP_CONST          (0)
#define MICROPY_MEM_STATS           (0)
#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_READER_POSIX        (0)	/* a 1 uses system calls (open() etc) */
#define MICROPY_KBD_EXCEPTION       (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_HELPER_LEXER_UNIX   (1)
#define MICROPY_ENABLE_SOURCE_LINE  (0) /* CSRB: set? */
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_WARNINGS            (0)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF   (0)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_NONE)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_NONE)
#define MICROPY_STREAMS_NON_BLOCK   (0)
#define MICROPY_OPT_COMPUTED_GOTO   (0)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (0)
#define MICROPY_CAN_OVERRIDE_BUILTINS (0)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_CPYTHON_COMPAT      (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (0)
#define MICROPY_PY_BUILTINS_COMPILE (0)
#define MICROPY_PY_BUILTINS_ENUMERATE (0)
#define MICROPY_PY_BUILTINS_FILTER  (0)
#define MICROPY_PY_BUILTINS_FROZENSET (0)
#define MICROPY_PY_BUILTINS_REVERSED (0)
#define MICROPY_PY_BUILTINS_SET     (0)
#define MICROPY_PY_BUILTINS_SLICE   (0)
#define MICROPY_PY_BUILTINS_STR_UNICODE (0)
#define MICROPY_PY_BUILTINS_PROPERTY (0)
#define MICROPY_PY_BUILTINS_MIN_MAX (0)
#define MICROPY_PY___FILE__         (0)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (0)
#define MICROPY_PY_GC               (0)
#define MICROPY_PY_GC_COLLECT_RETVAL (0)
#define MICROPY_PY_ARRAY            (1)
#define MICROPY_PY_COLLECTIONS      (1)
#define MICROPY_PY_MATH             (1)
#define MICROPY_PY_STRUCT           (1)
#define MICROPY_PY_SYS              (0)
#define MICROPY_PY_SYS_EXIT         (0)
#define MICROPY_PY_SYS_PLATFORM     "CSRB"
#define MICROPY_PY_SYS_MAXSIZE      (0)
#define MICROPY_PY_SYS_STDFILES     (0)
#define MICROPY_PY_CMATH            (1)
#define MICROPY_PY_UCTYPES          (1)
#define MICROPY_PY_UZLIB            (1)
#define MICROPY_PY_UJSON            (1)
#define MICROPY_PY_URE              (0)
#define MICROPY_PY_UHEAPQ           (0)
#define MICROPY_PY_UHASHLIB         (1)
#define MICROPY_PY_UBINASCII        (1)

/* py/modio.c */
#define MICROPY_PY_IO		(1) /* needed for VFS_X */
#define MICROPY_PY_IO_IOBASE	(1)
#define MICROPY_PY_IO_FILEIO	(1) /* used also by VFS_CSRB */
#define MICROPY_PY_IO_BYTESIO	(1)

/* extmod/vfs.c */
#define MICROPY_VFS       	(1)
#define MICROPY_VFS_CSRB       	(1)

#define MICROPY_READER_VFS     	(1)

#define mp_builtin_open_obj mp_vfs_open_obj
#define MICROPY_PORT_BUILTINS \
{ MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },
#define mp_type_fileio mp_type_vfs_csrb_fileio

extern const struct _mp_obj_module_t mp_module_os;

#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_uos), MP_ROM_PTR(&mp_module_os) }, \

#define MICROPY_PORT_ROOT_POINTERS \

//////////////////////////////////////////
// Do not change anything beyond this line
//////////////////////////////////////////

// Define to 1 to use undertested inefficient GC helper implementation
// (if more efficient arch-specific one is not available).
#ifndef MICROPY_GCREGS_SETJMP
    #ifdef __mips__
        #define MICROPY_GCREGS_SETJMP (1)
    #else
        #define MICROPY_GCREGS_SETJMP (0)
    #endif
#endif

// type definitions for the specific machine

#ifdef __LP64__
typedef long mp_int_t; // must be pointer size
typedef unsigned long mp_uint_t; // must be pointer size
#else
// These are definitions for machines where sizeof(int) == sizeof(void*),
// regardless for actual size.
typedef int mp_int_t; // must be pointer size
typedef unsigned int mp_uint_t; // must be pointer size
#endif

// Cannot include <sys/types.h>, as it may lead to symbol name clashes
#if _FILE_OFFSET_BITS == 64 && !defined(__LP64__)
typedef long long mp_off_t;
#else
typedef long mp_off_t;
#endif

// We need to provide a declaration/definition of alloca()
#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <alloca.h>
#endif


/* disable the redirection of the system's printf to MP's functions */
#define MICROPY_USE_INTERNAL_PRINTF	(0)

#include <stdint.h>
extern "C" void mp_csrb_print_strn(const char *str, const uint32_t strSize);
#define MP_PLAT_PRINT_STRN(str, len) mp_csrb_print_strn(str, len)
