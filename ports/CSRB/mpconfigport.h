#define MICROPY_ALLOC_PATH_MAX      (PATH_MAX)
#define MICROPY_ENABLE_GC           (1) /* CSRB: TODO CHECK */
#define MICROPY_ENABLE_FINALISER    (0)
/* Python call frames come from a dedicated LIFO arena instead of the GC heap:
 * cheaper to allocate, freed on exception unwind by nlr, and call depth is
 * bounded by the arena instead of by heap pressure.  Generators are unaffected
 * - their state outlives the call, so it stays in the GC heap.  The arena is
 * set up in mp_CSRB_init(), which runs before mp_init() as required. */
#define MICROPY_ENABLE_PYSTACK      (1)
/* Without this, runaway recursion in a script walks off the C stack and takes
 * the process down with it.  The embedder has to record a stack top with
 * mp_stack_ctrl_init() on the thread that runs the VM, and set a limit that
 * leaves room for the deepest frame the checks cannot see between them. */
#define MICROPY_STACK_CHECK         (1)
#define MICROPY_COMP_CONST          (1)
#define MICROPY_MEM_STATS           (0)
#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_READER_POSIX        (0) /* a 1 uses system calls (open() etc) */
#define MICROPY_KBD_EXCEPTION       (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_HELPER_LEXER_UNIX   (0) /* 1: needs mp_reader_new_file_from_fd() */
#define MICROPY_ENABLE_SOURCE_LINE  (1)
/* The console buffer is a remote author's only diagnostic channel, so spend
 * the code size on real error messages. */
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_WARNINGS            (0)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF   (0)
/* Scripts arrive from authors who expect Python: without floats any numeric
 * work fails outright, and without arbitrary precision ints anything past 63
 * bits - hashes, IDs, timestamp arithmetic - raises an overflow. */
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_STREAMS_NON_BLOCK   (0)
#define MICROPY_OPT_COMPUTED_GOTO   (1)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1)
#define MICROPY_CAN_OVERRIDE_BUILTINS (0)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_CPYTHON_COMPAT      (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (0)
#define MICROPY_PY_BUILTINS_COMPILE (0)
#define MICROPY_PY_BUILTINS_ENUMERATE (1)
#define MICROPY_PY_BUILTINS_FILTER  (1)
#define MICROPY_PY_BUILTINS_FROZENSET (1)
#define MICROPY_PY_BUILTINS_REVERSED (1)
#define MICROPY_PY_BUILTINS_SET     (1)
#define MICROPY_PY_BUILTINS_SLICE   (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE (1)
#define MICROPY_PY_BUILTINS_PROPERTY (1)
#define MICROPY_PY_BUILTINS_MIN_MAX (1)
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

/* mpconfigport_coverage.h */
#define MICROPY_PY_BUILTINS_HELP       (1)
#define MICROPY_PY_BUILTINS_HELP_MODULES (1)

/* Bound how long a single execution may run.  Scripts arrive from the network,
 * so without this a "while True: pass" would hold the interpreter - which is
 * process global, and therefore serialised by the caller - for ever.
 *
 * mp_csrb_vm_hook() raises KeyboardInterrupt once the deadline armed by
 * mp_CSRB_execution_begin() passes; MICROPY_KBD_EXCEPTION above gives it a
 * preallocated exception to raise.  The divisor keeps the clock read off the
 * hot path: the hook sits at the pending exception check, which every
 * backwards jump passes through - DISPATCH_WITH_PEND_EXC_CHECK() is "goto
 * pending_exception_check" under both dispatch modes, so this holds with
 * MICROPY_OPT_COMPUTED_GOTO on as well. */
#define MICROPY_VM_HOOK_COUNT       (4096)
#define MICROPY_VM_HOOK_INIT        uint32_t vm_hook_divisor = MICROPY_VM_HOOK_COUNT;
#define MICROPY_VM_HOOK_POLL        if(--vm_hook_divisor == 0) { \
        vm_hook_divisor = MICROPY_VM_HOOK_COUNT; \
        mp_csrb_vm_hook(); \
    }
#define MICROPY_VM_HOOK_LOOP        MICROPY_VM_HOOK_POLL
#define MICROPY_VM_HOOK_RETURN      MICROPY_VM_HOOK_POLL

/* enable module thread and support thread safety */
#define MICROPY_PY_THREAD           (0) /* TODO: needs porting */
/* enabme internal thread synchronization */
#define MICROPY_PY_THREAD_GIL       (0) /* TODO: needs porting */

/* py/modio.c */
#define MICROPY_PY_IO               (1) /* needed for mp_type_stringio */
#define MICROPY_PY_IO_IOBASE        (0) /* handled by VFS_CSRB */
#define MICROPY_PY_IO_BUFFEREDWRITER (0) /* handled by VFS_CSRB */
#define MICROPY_PY_IO_FILEIO        (0) /* handled by VFS_CSRB */
#define MICROPY_PY_IO_BYTESIO       (0) /* handled by VFS_CSRB */

/* extmod/vfs.c */
#define MICROPY_VFS                 (1)
#define MICROPY_VFS_CSRB            (1)

#define MICROPY_READER_VFS          (1)

#define MICROPY_ENABLE_EXTERNAL_IMPORT        (0) /* disable importing of external modules at runtime */

#define mp_builtin_open_obj mp_vfs_open_obj
#define MICROPY_PORT_BUILTINS \
{ MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) }, \
{ MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&mp_vfs_listdir_obj) }, \
{ MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&mp_vfs_stat_obj) }, \


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
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <stdlib.h>
#else
#include <alloca.h>
#endif


/* disable the redirection of the system's printf to MP's functions */
#define MICROPY_USE_INTERNAL_PRINTF        (0)

extern "C++"
{
#include <mp_csrb.h>
}
