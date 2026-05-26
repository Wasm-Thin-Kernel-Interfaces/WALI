from __future__ import annotations

from dataclasses import dataclass, field
from typing import List, Tuple, Optional, NamedTuple, Dict
from .abi import TypeRegistry

@dataclass
class ArchNrs:
    x86_64: int = -1
    arm64: int = -1
    rv64: int = -1

class SyscallArg(str):
    def is_ptr(self) -> bool:
        return self.endswith('*')

    def is_fn_ptr(self) -> bool:
        return self.startswith('fn(')

    def ptr_split(self, max: int | None = None) -> Tuple[int, 'SyscallArg']:
        """Returns a split of the number of pointer indirections and base type."""
        a = self.rstrip('*')
        indirection = len(self) - len(a)
        if max is not None and indirection > max:
            a += '*' * (indirection - max)
        return (len(self) - len(a), SyscallArg(a))



@dataclass
class Syscall:
    name: str
    nrs: ArchNrs
    # Stores just the types of the arguments
    args: List[SyscallArg] = field(default_factory=list)
    # Stores the identifier names accompanying the types
    args_id: List[str] = field(default_factory=list)
    aliases: List[str] = field(default_factory=list)
    unused_aliases: List[str] = field(default_factory=list)
    implemented: bool = False
    
    @property
    def nr(self) -> int:
        """Helper to get x86_64 number (default conventions usually refer to this)"""
        return self.nrs.x86_64

    @property
    def num_args(self) -> int:
        return len(self.args)

    def args_reduce(self) -> List[SyscallArg]:
        '''Returns argument list with complex types replaced by their base types.'''
        def f(arg: SyscallArg):
            try:
                return SyscallArg(TypeRegistry.reduce_alias(arg))
            except KeyError as e:
                raise RuntimeError(f"Type '{arg}' not found in type aliases (used in syscall '{self.name}')") from e

        return [f(arg) if not arg.is_ptr() and not TypeRegistry.is_primitive(arg) else arg for arg in self.args]


# -----------------------------------------------------------------------------
# Helper Functions
# -----------------------------------------------------------------------------

def split_args(a: str) -> Tuple[SyscallArg, str]:
    parts = a.rsplit(maxsplit=1)
    if len(parts) != 2:
        raise RuntimeError(f"Argument '{a}' is not in the expected format '<type> <id>'")
    return (SyscallArg(parts[0]), parts[1])


def unimpl(name: str, nrs: ArchNrs) -> Syscall:
    """Shorthand for syscalls with no argument info or aliases (unimplemented/simple)."""
    return Syscall(name=name, nrs=nrs)

def impl(name: str, args: List[str], nrs: ArchNrs, **kwargs) -> Syscall:
    """Full creation helper for fully defined (implemented) syscalls."""
    args_ty, args_id = map(list, zip(*[split_args(a) for a in args])) if args else ([], [])
    return Syscall(name=name, nrs=nrs, args=args_ty, args_id=args_id, implemented=True, **kwargs)

# -----------------------------------------------------------------------------
# Definitions
# -----------------------------------------------------------------------------

_syscall_list: List[Syscall] = [
    impl("read", ["int fd", "void* buf", "size_t count"], ArchNrs(x86_64=0, arm64=63, rv64=63)),
    impl("write", ["int fd", "void* buf", "size_t count"], ArchNrs(x86_64=1, arm64=64, rv64=64)),
    impl("open", ["char* pathname", "int flags", "mode_t mode"], ArchNrs(x86_64=2)),
    impl("close", ["int fd"], ArchNrs(x86_64=3, arm64=57, rv64=57)),
    impl("stat", ["char* pathname", "struct stat* statbuf"], ArchNrs(x86_64=4)),
    impl("fstat", ["int fd", "struct stat* statbuf"], ArchNrs(x86_64=5, arm64=80, rv64=80)),
    impl("lstat", ["char* pathname", "struct stat* statbuf"], ArchNrs(x86_64=6)),
    impl("poll", ["struct pollfd* fds", "nfds_t nfds", "int timeout"], ArchNrs(x86_64=7)),
    impl("lseek", ["int fd", "off_t offset", "int whence"], ArchNrs(x86_64=8, arm64=62, rv64=62)),
    impl("mmap", ["void* addr", "size_t length", "int prot", "int flags", "int fd", "off_t offset"], ArchNrs(x86_64=9, arm64=222, rv64=222), unused_aliases=["mmap2"]),
    impl("mprotect", ["void* addr", "size_t len", "int prot"], ArchNrs(x86_64=10, arm64=226, rv64=226)),
    impl("munmap", ["void* addr", "size_t len"], ArchNrs(x86_64=11, arm64=215, rv64=215)),
    impl("brk", ["void* addr"], ArchNrs(x86_64=12, arm64=214, rv64=214)),
    impl("rt_sigaction", ["int signum", "struct sigaction* act", "struct sigaction* oldact", "size_t sigsetsize"], ArchNrs(x86_64=13, arm64=134, rv64=134)),
    impl("rt_sigprocmask", ["int how", "sigset_t* set", "sigset_t* oldset", "size_t sigsetsize"], ArchNrs(x86_64=14, arm64=135, rv64=135)),
    impl("rt_sigreturn", ["long unused"], ArchNrs(x86_64=15, arm64=139, rv64=139)),
    impl("ioctl", ["int fd", "int request", "char* argp"], ArchNrs(x86_64=16, arm64=29, rv64=29)),
    impl("pread64", ["int fd", "char* buf", "size_t count", "off_t offset"], ArchNrs(x86_64=17, arm64=67, rv64=67)),
    impl("pwrite64", ["int fd", "char* buf", "size_t count", "off_t offset"], ArchNrs(x86_64=18, arm64=68, rv64=68)),
    impl("readv", ["int fd", "struct iovec* iov", "int iovcnt"], ArchNrs(x86_64=19, arm64=65, rv64=65)),
    impl("writev", ["int fd", "struct iovec* iov", "int iovcnt"], ArchNrs(x86_64=20, arm64=66, rv64=66)),
    impl("access", ["char* pathname", "int mode"], ArchNrs(x86_64=21)),
    impl("pipe", ["int* pipefd"], ArchNrs(x86_64=22)),
    impl("select", ["int nfds", "fd_set* readfds", "fd_set* writefds", "fd_set* exceptfds", "struct timeval* timeout"], ArchNrs(x86_64=23)),
    impl("sched_yield", [], ArchNrs(x86_64=24, arm64=124, rv64=124)),
    impl("mremap", ["void* old_address", "size_t old_size", "size_t new_size", "int flags", "void* new_address"], ArchNrs(x86_64=25, arm64=216, rv64=216)),
    impl("msync", ["void* addr", "size_t length", "int flags"], ArchNrs(x86_64=26, arm64=227, rv64=227)),
    unimpl("mincore", ArchNrs(x86_64=27, arm64=232, rv64=232)),
    impl("madvise", ["void* addr", "size_t length", "int advice"], ArchNrs(x86_64=28, arm64=233, rv64=233)),
    unimpl("shmget", ArchNrs(x86_64=29, arm64=194, rv64=194)),
    unimpl("shmat", ArchNrs(x86_64=30, arm64=196, rv64=196)),
    unimpl("shmctl", ArchNrs(x86_64=31, arm64=195, rv64=195)),
    impl("dup", ["int oldfd"], ArchNrs(x86_64=32, arm64=23, rv64=23)),
    impl("dup2", ["int oldfd", "int newfd"], ArchNrs(x86_64=33)),
    unimpl("pause", ArchNrs(x86_64=34)),
    impl("nanosleep", ["struct timespec* req", "struct timespec* rem"], ArchNrs(x86_64=35, arm64=101, rv64=101)),
    unimpl("getitimer", ArchNrs(x86_64=36, arm64=102, rv64=102)),
    unimpl("alarm", ArchNrs(x86_64=37)),
    impl("setitimer", ["int which", "struct itimerval* new_value", "struct itimerval* old_value"], ArchNrs(x86_64=38, arm64=103, rv64=103)),
    impl("getpid", [], ArchNrs(x86_64=39, arm64=172, rv64=172)),
    unimpl("sendfile", ArchNrs(x86_64=40, arm64=71, rv64=71)),
    impl("socket", ["int domain", "int type", "int protocol"], ArchNrs(x86_64=41, arm64=198, rv64=198)),
    impl("connect", ["int sockfd", "struct sockaddr* addr", "socklen_t addrlen"], ArchNrs(x86_64=42, arm64=203, rv64=203)),
    impl("accept", ["int sockfd", "struct sockaddr* addr", "socklen_t* addrlen"], ArchNrs(x86_64=43, arm64=202, rv64=202)),
    impl("sendto", ["int sockfd", "void* buf", "size_t len", "int flags", "struct sockaddr* dest_addr", "socklen_t addrlen"], ArchNrs(x86_64=44, arm64=206, rv64=206)),
    impl("recvfrom", ["int sockfd", "void* buf", "size_t len", "int flags", "struct sockaddr* src_addr", "socklen_t* addrlen"], ArchNrs(x86_64=45, arm64=207, rv64=207)),
    impl("sendmsg", ["int sockfd", "struct msghdr* msg", "int flags"], ArchNrs(x86_64=46, arm64=211, rv64=211)),
    impl("recvmsg", ["int sockfd", "struct msghdr* msg", "int flags"], ArchNrs(x86_64=47, arm64=212, rv64=212)),
    impl("shutdown", ["int sockfd", "int how"], ArchNrs(x86_64=48, arm64=210, rv64=210)),
    impl("bind", ["int sockfd", "struct sockaddr* addr", "socklen_t addrlen"], ArchNrs(x86_64=49, arm64=200, rv64=200)),
    impl("listen", ["int sockfd", "int backlog"], ArchNrs(x86_64=50, arm64=201, rv64=201)),
    impl("getsockname", ["int sockfd", "struct sockaddr* addr", "socklen_t* addrlen"], ArchNrs(x86_64=51, arm64=204, rv64=204)),
    impl("getpeername", ["int sockfd", "struct sockaddr* addr", "socklen_t* addrlen"], ArchNrs(x86_64=52, arm64=205, rv64=205)),
    impl("socketpair", ["int domain", "int type", "int protocol", "int* sv"], ArchNrs(x86_64=53, arm64=199, rv64=199)),
    impl("setsockopt", ["int sockfd", "int level", "int optname", "void* optval", "socklen_t optlen"], ArchNrs(x86_64=54, arm64=208, rv64=208)),
    impl("getsockopt", ["int sockfd", "int level", "int optname", "void* optval", "socklen_t* optlen"], ArchNrs(x86_64=55, arm64=209, rv64=209)),
    unimpl("clone", ArchNrs(x86_64=56, arm64=220, rv64=220)),
    impl("fork", [], ArchNrs(x86_64=57)),
    unimpl("vfork", ArchNrs(x86_64=58)),
    impl("execve", ["char* pathname", "char** argv", "char** envp"], ArchNrs(x86_64=59, arm64=221, rv64=221)),
    impl("exit", ["int status"], ArchNrs(x86_64=60, arm64=93, rv64=93)),
    impl("wait4", ["pid_t pid", "int* wstatus", "int options", "struct rusage* rusage"], ArchNrs(x86_64=61, arm64=260, rv64=260)),
    impl("kill", ["pid_t pid", "int sig"], ArchNrs(x86_64=62, arm64=129, rv64=129)),
    impl("uname", ["struct utsname* buf"], ArchNrs(x86_64=63, arm64=160, rv64=160)),
    unimpl("semget", ArchNrs(x86_64=64, arm64=190, rv64=190)),
    unimpl("semop", ArchNrs(x86_64=65, arm64=193, rv64=193)),
    unimpl("semctl", ArchNrs(x86_64=66, arm64=191, rv64=191)),
    unimpl("shmdt", ArchNrs(x86_64=67, arm64=197, rv64=197)),
    unimpl("msgget", ArchNrs(x86_64=68, arm64=186, rv64=186)),
    unimpl("msgsnd", ArchNrs(x86_64=69, arm64=189, rv64=189)),
    unimpl("msgrcv", ArchNrs(x86_64=70, arm64=188, rv64=188)),
    unimpl("msgctl", ArchNrs(x86_64=71, arm64=187, rv64=187)),
    # fcntl flags only span 32-bits in wasm32, but conservatively use 64-bit value
    impl("fcntl", ["int fd", "int cmd", "unsigned long arg"], ArchNrs(x86_64=72, arm64=25, rv64=25)),
    impl("flock", ["int fd", "int operation"], ArchNrs(x86_64=73, arm64=32, rv64=32)),
    impl("fsync", ["int fd"], ArchNrs(x86_64=74, arm64=82, rv64=82)),
    impl("fdatasync", ["int fd"], ArchNrs(x86_64=75, arm64=83, rv64=83)),
    unimpl("truncate", ArchNrs(x86_64=76, arm64=45, rv64=45)),
    impl("ftruncate", ["int fd", "off_t length"], ArchNrs(x86_64=77, arm64=46, rv64=46)),
    # getdents is obsolete and legacy; it is aliased to getdents64 for WALI
    unimpl("getdents", ArchNrs(x86_64=78)),
    impl("getcwd", ["char* buf", "size_t size"], ArchNrs(x86_64=79, arm64=17, rv64=17)),
    impl("chdir", ["char* path"], ArchNrs(x86_64=80, arm64=49, rv64=49)),
    impl("fchdir", ["int fd"], ArchNrs(x86_64=81, arm64=50, rv64=50)),
    impl("rename", ["char* oldpath", "char* newpath"], ArchNrs(x86_64=82)),
    impl("mkdir", ["char* pathname", "mode_t mode"], ArchNrs(x86_64=83)),
    impl("rmdir", ["char* pathname"], ArchNrs(x86_64=84)),
    unimpl("creat", ArchNrs(x86_64=85)),
    impl("link", ["char* oldpath", "char* newpath"], ArchNrs(x86_64=86)),
    impl("unlink", ["char* pathname"], ArchNrs(x86_64=87)),
    impl("symlink", ["char* target", "char* linkpath"], ArchNrs(x86_64=88)),
    impl("readlink", ["char* pathname", "char* buf", "size_t bufsiz"], ArchNrs(x86_64=89)),
    impl("chmod", ["char* pathname", "mode_t mode"], ArchNrs(x86_64=90)),
    impl("fchmod", ["int fd", "mode_t mode"], ArchNrs(x86_64=91, arm64=52, rv64=52)),
    impl("chown", ["char* pathname", "uid_t owner", "gid_t group"], ArchNrs(x86_64=92)),
    impl("fchown", ["int fd", "uid_t owner", "gid_t group"], ArchNrs(x86_64=93, arm64=55, rv64=55)),
    unimpl("lchown", ArchNrs(x86_64=94)),
    impl("umask", ["mode_t mask"], ArchNrs(x86_64=95, arm64=166, rv64=166)),
    impl("gettimeofday", ["struct timeval* tv", "struct timezone* tz"], ArchNrs(x86_64=96, arm64=169, rv64=169)),
    impl("getrlimit", ["int resource", "struct rlimit* rlim"], ArchNrs(x86_64=97, arm64=163, rv64=163)),
    impl("getrusage", ["int who", "struct rusage* usage"], ArchNrs(x86_64=98, arm64=165, rv64=165)),
    impl("sysinfo", ["struct sysinfo* info"], ArchNrs(x86_64=99, arm64=179, rv64=179)),
    unimpl("times", ArchNrs(x86_64=100, arm64=153, rv64=153)),
    unimpl("ptrace", ArchNrs(x86_64=101, arm64=117, rv64=117)),
    impl("getuid", [], ArchNrs(x86_64=102, arm64=174, rv64=174)),
    unimpl("syslog", ArchNrs(x86_64=103, arm64=116, rv64=116)),
    impl("getgid", [], ArchNrs(x86_64=104, arm64=176, rv64=176)),
    impl("setuid", ["uid_t uid"], ArchNrs(x86_64=105, arm64=146, rv64=146)),
    impl("setgid", ["gid_t gid"], ArchNrs(x86_64=106, arm64=144, rv64=144)),
    impl("geteuid", [], ArchNrs(x86_64=107, arm64=175, rv64=175)),
    impl("getegid", [], ArchNrs(x86_64=108, arm64=177, rv64=177)),
    impl("setpgid", ["pid_t pid", "pid_t pgid"], ArchNrs(x86_64=109, arm64=154, rv64=154)),
    impl("getppid", [], ArchNrs(x86_64=110, arm64=173, rv64=173)),
    unimpl("getpgrp", ArchNrs(x86_64=111, arm64=151, rv64=151)),
    impl("setsid", [], ArchNrs(x86_64=112, arm64=157, rv64=157)),
    impl("setreuid", ["uid_t ruid", "uid_t euid"], ArchNrs(x86_64=113, arm64=145, rv64=145)),
    impl("setregid", ["gid_t rgid", "gid_t egid"], ArchNrs(x86_64=114, arm64=143, rv64=143)),
    impl("getgroups", ["size_t size", "gid_t* list"], ArchNrs(x86_64=115, arm64=158, rv64=158)),
    impl("setgroups", ["size_t size", "gid_t* list"], ArchNrs(x86_64=116, arm64=159, rv64=159)),
    impl("setresuid", ["uid_t ruid", "uid_t euid", "uid_t suid"], ArchNrs(x86_64=117, arm64=147, rv64=147)),
    unimpl("getresuid", ArchNrs(x86_64=118, arm64=148, rv64=148)),
    impl("setresgid", ["gid_t rgid", "gid_t egid", "gid_t sgid"], ArchNrs(x86_64=119, arm64=149, rv64=149)),
    unimpl("getresgid", ArchNrs(x86_64=120, arm64=150, rv64=150)),
    impl("getpgid", ["pid_t pid"], ArchNrs(x86_64=121, arm64=155, rv64=155)),
    unimpl("setfsuid", ArchNrs(x86_64=122, arm64=151, rv64=151)),
    unimpl("setfsgid", ArchNrs(x86_64=123, arm64=152, rv64=152)),
    impl("getsid", ["pid_t pid"], ArchNrs(x86_64=124, arm64=156, rv64=156)),
    unimpl("capget", ArchNrs(x86_64=125, arm64=90, rv64=90)),
    unimpl("capset", ArchNrs(x86_64=126, arm64=91, rv64=91)),
    impl("rt_sigpending", ["sigset_t* set", "size_t sigsetsize"], ArchNrs(x86_64=127, arm64=136, rv64=136)),
    unimpl("rt_sigtimedwait", ArchNrs(x86_64=128, arm64=137, rv64=137)),
    unimpl("rt_sigqueueinfo", ArchNrs(x86_64=129, arm64=138, rv64=138)),
    impl("rt_sigsuspend", ["sigset_t* mask", "size_t sigsetsize"],ArchNrs(x86_64=130, arm64=133, rv64=133)),
    impl("sigaltstack", ["stack_t* ss", "stack_t* old_ss"], ArchNrs(x86_64=131, arm64=132, rv64=132)),
    # utime is a legacy call; use utimensat which WALI implements
    unimpl("utime", ArchNrs(x86_64=132)),
    unimpl("mknod", ArchNrs(x86_64=133)),
    unimpl("uselib", ArchNrs(x86_64=134)),
    unimpl("personality", ArchNrs(x86_64=135, arm64=92, rv64=92)),
    unimpl("ustat", ArchNrs(x86_64=136)),
    impl("statfs", ["char* path", "struct statfs* buf"], ArchNrs(x86_64=137, arm64=43, rv64=43)),
    impl("fstatfs", ["int fd", "struct statfs* buf"], ArchNrs(x86_64=138, arm64=44, rv64=44)),
    unimpl("sysfs", ArchNrs(x86_64=139)),
    unimpl("getpriority", ArchNrs(x86_64=140, arm64=141, rv64=141)),
    unimpl("setpriority", ArchNrs(x86_64=141, arm64=140, rv64=140)),
    unimpl("sched_setparam", ArchNrs(x86_64=142, arm64=118, rv64=118)),
    unimpl("sched_getparam", ArchNrs(x86_64=143, arm64=121, rv64=121)),
    unimpl("sched_setscheduler", ArchNrs(x86_64=144, arm64=119, rv64=119)),
    unimpl("sched_getscheduler", ArchNrs(x86_64=145, arm64=120, rv64=120)),
    unimpl("sched_get_priority_max", ArchNrs(x86_64=146, arm64=125, rv64=125)),
    unimpl("sched_get_priority_min", ArchNrs(x86_64=147, arm64=126, rv64=126)),
    unimpl("sched_rr_get_interval", ArchNrs(x86_64=148, arm64=127, rv64=127)),
    unimpl("mlock", ArchNrs(x86_64=149, arm64=228, rv64=228)),
    unimpl("munlock", ArchNrs(x86_64=150, arm64=229, rv64=229)),
    unimpl("mlockall", ArchNrs(x86_64=151, arm64=230, rv64=230)),
    unimpl("munlockall", ArchNrs(x86_64=152, arm64=231, rv64=231)),
    unimpl("vhangup", ArchNrs(x86_64=153, arm64=58, rv64=58)),
    unimpl("modify_ldt", ArchNrs(x86_64=154)),
    unimpl("pivot_root", ArchNrs(x86_64=155, arm64=41, rv64=41)),
    unimpl("_sysctl", ArchNrs(x86_64=156)),
    impl("prctl", ["int option", "unsigned long arg2", "unsigned long arg3", "unsigned long arg4", "unsigned long arg5"], ArchNrs(x86_64=157, arm64=167, rv64=167)),
    unimpl("arch_prctl", ArchNrs(x86_64=158)),
    unimpl("adjtimex", ArchNrs(x86_64=159, arm64=171, rv64=171)),
    impl("setrlimit", ["int resource", "struct rlimit* rlim"], ArchNrs(x86_64=160, arm64=164, rv64=164)),
    impl("chroot", ["char* path"], ArchNrs(x86_64=161, arm64=51, rv64=51)),
    unimpl("sync", ArchNrs(x86_64=162, arm64=81, rv64=81)),
    unimpl("acct", ArchNrs(x86_64=163, arm64=89, rv64=89)),
    unimpl("settimeofday", ArchNrs(x86_64=164, arm64=170, rv64=170)),
    unimpl("mount", ArchNrs(x86_64=165, arm64=40, rv64=40)),
    unimpl("umount2", ArchNrs(x86_64=166, arm64=39, rv64=39)),
    unimpl("swapon", ArchNrs(x86_64=167, arm64=224, rv64=224)),
    unimpl("swapoff", ArchNrs(x86_64=168, arm64=225, rv64=225)),
    unimpl("reboot", ArchNrs(x86_64=169, arm64=142, rv64=142)),
    unimpl("sethostname", ArchNrs(x86_64=170, arm64=161, rv64=161)),
    unimpl("setdomainname", ArchNrs(x86_64=171, arm64=162, rv64=162)),
    unimpl("iopl", ArchNrs(x86_64=172)),
    unimpl("ioperm", ArchNrs(x86_64=173)),
    unimpl("create_module", ArchNrs(x86_64=174)),
    unimpl("init_module", ArchNrs(x86_64=175, arm64=105, rv64=105)),
    unimpl("delete_module", ArchNrs(x86_64=176, arm64=106, rv64=106)),
    unimpl("get_kernel_syms", ArchNrs(x86_64=177)),
    unimpl("query_module", ArchNrs(x86_64=178)),
    unimpl("quotactl", ArchNrs(x86_64=179, arm64=60, rv64=60)),
    # 42 is deprecated for rv64, so don't include it here
    unimpl("nfsservctl", ArchNrs(x86_64=180, arm64=42)),
    unimpl("getpmsg", ArchNrs(x86_64=181)),
    unimpl("putpmsg", ArchNrs(x86_64=182)),
    unimpl("afs_syscall", ArchNrs(x86_64=183)),
    unimpl("tuxcall", ArchNrs(x86_64=184)),
    unimpl("security", ArchNrs(x86_64=185)),
    impl("gettid", [], ArchNrs(x86_64=186, arm64=178, rv64=178)),
    unimpl("readahead", ArchNrs(x86_64=187, arm64=213, rv64=213)),
    unimpl("setxattr", ArchNrs(x86_64=188, arm64=5, rv64=5)),
    unimpl("lsetxattr", ArchNrs(x86_64=189, arm64=6, rv64=6)),
    unimpl("fsetxattr", ArchNrs(x86_64=190, arm64=7, rv64=7)),
    unimpl("getxattr", ArchNrs(x86_64=191, arm64=8, rv64=8)),
    unimpl("lgetxattr", ArchNrs(x86_64=192, arm64=9, rv64=9)),
    unimpl("fgetxattr", ArchNrs(x86_64=193, arm64=10, rv64=10)),
    unimpl("listxattr", ArchNrs(x86_64=194, arm64=11, rv64=11)),
    unimpl("llistxattr", ArchNrs(x86_64=195, arm64=12, rv64=12)),
    unimpl("flistxattr", ArchNrs(x86_64=196, arm64=13, rv64=13)),
    unimpl("removexattr", ArchNrs(x86_64=197, arm64=14, rv64=14)),
    unimpl("lremovexattr", ArchNrs(x86_64=198, arm64=15, rv64=15)),
    unimpl("fremovexattr", ArchNrs(x86_64=199, arm64=16, rv64=16)),
    impl("tkill", ["int tid", "int sig"], ArchNrs(x86_64=200, arm64=130, rv64=130)),
    unimpl("time", ArchNrs(x86_64=201)),
    impl("futex", ["int* uaddr", "int futex_op", "int val", "struct timespec* timeout", "int* uaddr2", "int val3"], ArchNrs(x86_64=202, arm64=98, rv64=98)),
    unimpl("sched_setaffinity", ArchNrs(x86_64=203, arm64=122, rv64=122)),
    impl("sched_getaffinity", ["pid_t pid", "size_t cpusetsize", "cpu_set_t* mask"], ArchNrs(x86_64=204, arm64=123, rv64=123)),
    unimpl("set_thread_area", ArchNrs(x86_64=205)),
    unimpl("io_setup", ArchNrs(x86_64=206, arm64=0, rv64=0)),
    unimpl("io_destroy", ArchNrs(x86_64=207, arm64=1, rv64=1)),
    unimpl("io_getevents", ArchNrs(x86_64=208, arm64=4, rv64=4)),
    unimpl("io_submit", ArchNrs(x86_64=209, arm64=2, rv64=2)),
    unimpl("io_cancel", ArchNrs(x86_64=210, arm64=3, rv64=3)),
    unimpl("get_thread_area", ArchNrs(x86_64=211)),
    unimpl("lookup_dcookie", ArchNrs(x86_64=212, arm64=18, rv64=18)),
    unimpl("epoll_create", ArchNrs(x86_64=213)),
    unimpl("epoll_ctl_old", ArchNrs(x86_64=214)),
    unimpl("epoll_wait_old", ArchNrs(x86_64=215)),
    unimpl("remap_file_pages", ArchNrs(x86_64=216, arm64=234, rv64=234)),
    impl("getdents64", ["int fd", "struct dirent* dirp", "int count"], ArchNrs(x86_64=217, arm64=61, rv64=61), aliases=["getdents"]),
    impl("set_tid_address", ["int* tidptr"], ArchNrs(x86_64=218, arm64=96, rv64=96)),
    unimpl("restart_syscall", ArchNrs(x86_64=219, arm64=128, rv64=128)),
    unimpl("semtimedop", ArchNrs(x86_64=220, arm64=192, rv64=192)),
    impl("fadvise", ["int fd", "off_t offset", "off_t len", "int advice"], ArchNrs(x86_64=221, arm64=223, rv64=223), unused_aliases=["fadvise64"]),
    unimpl("timer_create", ArchNrs(x86_64=222, arm64=107, rv64=107)),
    unimpl("timer_settime", ArchNrs(x86_64=223, arm64=110, rv64=110)),
    unimpl("timer_gettime", ArchNrs(x86_64=224, arm64=108, rv64=108)),
    unimpl("timer_getoverrun", ArchNrs(x86_64=225, arm64=109, rv64=109)),
    unimpl("timer_delete", ArchNrs(x86_64=226, arm64=111, rv64=111)),
    unimpl("clock_settime", ArchNrs(x86_64=227, arm64=112, rv64=112)),
    impl("clock_gettime", ["clockid_t clockid", "struct timespec* tp"], ArchNrs(x86_64=228, arm64=113, rv64=113)),
    impl("clock_getres", ["clockid_t clockid", "struct timespec* res"], ArchNrs(x86_64=229, arm64=114, rv64=114)),
    impl("clock_nanosleep", ["clockid_t clockid", "int flags", "struct timespec* request", "struct timespec* remain"], ArchNrs(x86_64=230, arm64=115, rv64=115)),
    impl("exit_group", ["int status"], ArchNrs(x86_64=231, arm64=94, rv64=94)),
    unimpl("epoll_wait", ArchNrs(x86_64=232)),
    impl("epoll_ctl", ["int epfd", "int op", "int fd", "struct epoll_event* event"], ArchNrs(x86_64=233, arm64=21, rv64=21)),
    unimpl("tgkill", ArchNrs(x86_64=234, arm64=131, rv64=131)),
    unimpl("utimes", ArchNrs(x86_64=235)),
    unimpl("vserver", ArchNrs(x86_64=236)),
    unimpl("mbind", ArchNrs(x86_64=237, arm64=235, rv64=235)),
    unimpl("set_mempolicy", ArchNrs(x86_64=238, arm64=237, rv64=237)),
    unimpl("get_mempolicy", ArchNrs(x86_64=239, arm64=236, rv64=236)),
    unimpl("mq_open", ArchNrs(x86_64=240, arm64=180, rv64=180)),
    unimpl("mq_unlink", ArchNrs(x86_64=241, arm64=181, rv64=181)),
    unimpl("mq_timedsend", ArchNrs(x86_64=242, arm64=182, rv64=182)),
    unimpl("mq_timedreceive", ArchNrs(x86_64=243, arm64=183, rv64=183)),
    unimpl("mq_notify", ArchNrs(x86_64=244, arm64=184, rv64=184)),
    unimpl("mq_getsetattr", ArchNrs(x86_64=245, arm64=185, rv64=185)),
    unimpl("kexec_load", ArchNrs(x86_64=246, arm64=104, rv64=104)),
    unimpl("waitid", ArchNrs(x86_64=247, arm64=95, rv64=95)),
    unimpl("add_key", ArchNrs(x86_64=248, arm64=217, rv64=217)),
    unimpl("request_key", ArchNrs(x86_64=249, arm64=218, rv64=218)),
    unimpl("keyctl", ArchNrs(x86_64=250, arm64=219, rv64=219)),
    unimpl("ioprio_set", ArchNrs(x86_64=251, arm64=30, rv64=30)),
    unimpl("ioprio_get", ArchNrs(x86_64=252, arm64=31, rv64=31)),
    unimpl("inotify_init", ArchNrs(x86_64=253)),
    unimpl("inotify_add_watch", ArchNrs(x86_64=254, arm64=27, rv64=27)),
    unimpl("inotify_rm_watch", ArchNrs(x86_64=255, arm64=28, rv64=28)),
    unimpl("migrate_pages", ArchNrs(x86_64=256, arm64=238, rv64=238)),
    impl("openat", ["int dirfd", "char* pathname", "int flags", "mode_t mode"], ArchNrs(x86_64=257, arm64=56, rv64=56)),
    impl("mkdirat", ["int dirfd", "char* pathname", "mode_t mode"], ArchNrs(x86_64=258, arm64=34, rv64=34)),
    unimpl("mknodat", ArchNrs(x86_64=259, arm64=33, rv64=33)),
    impl("fchownat", ["int dirfd", "char* pathname", "uid_t owner", "gid_t group", "int flags"], ArchNrs(x86_64=260, arm64=54, rv64=54)),
    unimpl("futimesat", ArchNrs(x86_64=261)),
    impl("newfstatat", ["int dirfd", "char* pathname", "struct stat* statbuf", "int flags"], ArchNrs(x86_64=262, arm64=79, rv64=79), aliases=["fstatat"]),
    impl("unlinkat", ["int dirfd", "char* pathname", "int flags"], ArchNrs(x86_64=263, arm64=35, rv64=35)),
    unimpl("renameat", ArchNrs(x86_64=264, arm64=38, rv64=38)),
    impl("linkat", ["int olddirfd", "char* oldpath", "int newdirfd", "char* newpath", "int flags"], ArchNrs(x86_64=265, arm64=37, rv64=37)),
    impl("symlinkat", ["char* target", "int newdirfd", "char* linkpath"], ArchNrs(x86_64=266, arm64=36, rv64=36)),
    impl("readlinkat", ["int dirfd", "char* pathname", "char* buf", "size_t bufsiz"], ArchNrs(x86_64=267, arm64=78, rv64=78)),
    impl("fchmodat", ["int dirfd", "char* pathname", "mode_t mode", "int flags"], ArchNrs(x86_64=268, arm64=53, rv64=53)),
    impl("faccessat", ["int dirfd", "char* pathname", "int mode", "int flags"], ArchNrs(x86_64=269, arm64=48, rv64=48)),
    impl("pselect6", ["int nfds", "fd_set* readfds", "fd_set* writefds", "fd_set* exceptfds", "struct timespec* timeout", "void* sigmask"], ArchNrs(x86_64=270, arm64=72, rv64=72)),
    impl("ppoll", ["struct pollfd* fds", "nfds_t nfds", "struct timespec* tmo_p", "sigset_t* sigmask", "size_t sigsetsize"], ArchNrs(x86_64=271, arm64=73, rv64=73)),
    unimpl("unshare", ArchNrs(x86_64=272, arm64=97, rv64=97)),
    unimpl("set_robust_list", ArchNrs(x86_64=273, arm64=99, rv64=99)),
    unimpl("get_robust_list", ArchNrs(x86_64=274, arm64=100, rv64=100)),
    unimpl("splice", ArchNrs(x86_64=275, arm64=76, rv64=76)),
    unimpl("tee", ArchNrs(x86_64=276, arm64=77, rv64=77)),
    unimpl("sync_file_range", ArchNrs(x86_64=277, arm64=84, rv64=84)),
    unimpl("vmsplice", ArchNrs(x86_64=278, arm64=75, rv64=75)),
    unimpl("move_pages", ArchNrs(x86_64=279, arm64=239, rv64=239)),
    impl("utimensat", ["int dirfd", "char* pathname", "struct timespec* times", "int flags"], ArchNrs(x86_64=280, arm64=88, rv64=88)),
    impl("epoll_pwait", ["int epfd", "struct epoll_event* events", "int maxevents", "int timeout", "sigset_t* sigmask", "size_t sigsetsize"], ArchNrs(x86_64=281, arm64=22, rv64=22)),
    unimpl("signalfd", ArchNrs(x86_64=282)),
    unimpl("timerfd_create", ArchNrs(x86_64=283, arm64=85, rv64=85)),
    impl("eventfd", ["int initval"], ArchNrs(x86_64=284)),
    unimpl("fallocate", ArchNrs(x86_64=285, arm64=47, rv64=47)),
    unimpl("timerfd_settime", ArchNrs(x86_64=286, arm64=86, rv64=86)),
    unimpl("timerfd_gettime", ArchNrs(x86_64=287, arm64=87, rv64=87)),
    impl("accept4", ["int sockfd", "struct sockaddr* addr", "socklen_t* addrlen", "int flags"], ArchNrs(x86_64=288, arm64=242, rv64=242)),
    unimpl("signalfd4", ArchNrs(x86_64=289, arm64=74, rv64=74)),
    impl("eventfd2", ["int initval", "int flags"], ArchNrs(x86_64=290, arm64=19, rv64=19)),
    impl("epoll_create1", ["int flags"], ArchNrs(x86_64=291, arm64=20, rv64=20)),
    impl("dup3", ["int oldfd", "int newfd", "int flags"], ArchNrs(x86_64=292, arm64=24, rv64=24)),
    impl("pipe2", ["int* pipefd", "int flags"], ArchNrs(x86_64=293, arm64=59, rv64=59)),
    unimpl("inotify_init1", ArchNrs(x86_64=294, arm64=26, rv64=26)),
    unimpl("preadv", ArchNrs(x86_64=295, arm64=69, rv64=69)),
    unimpl("pwritev", ArchNrs(x86_64=296, arm64=70, rv64=70)),
    unimpl("rt_tgsigqueueinfo", ArchNrs(x86_64=297, arm64=240, rv64=240)),
    unimpl("perf_event_open", ArchNrs(x86_64=298, arm64=241, rv64=241)),
    unimpl("recvmmsg", ArchNrs(x86_64=299, arm64=243, rv64=243)),
    unimpl("fanotify_init", ArchNrs(x86_64=300, arm64=262, rv64=262)),
    unimpl("fanotify_mark", ArchNrs(x86_64=301, arm64=263, rv64=263)),
    impl("prlimit64", ["pid_t pid", "int resource", "struct rlimit* new_limit", "struct rlimit* old_limit"], ArchNrs(x86_64=302, arm64=261, rv64=261)),
    unimpl("name_to_handle_at", ArchNrs(x86_64=303, arm64=264, rv64=264)),
    unimpl("open_by_handle_at", ArchNrs(x86_64=304, arm64=265, rv64=265)),
    unimpl("clock_adjtime", ArchNrs(x86_64=305, arm64=266, rv64=266)),
    unimpl("syncfs", ArchNrs(x86_64=306, arm64=267, rv64=267)),
    unimpl("sendmmsg", ArchNrs(x86_64=307, arm64=269, rv64=269)),
    unimpl("setns", ArchNrs(x86_64=308, arm64=268, rv64=268)),
    unimpl("getcpu", ArchNrs(x86_64=309, arm64=168, rv64=168)),
    unimpl("process_vm_readv", ArchNrs(x86_64=310, arm64=270, rv64=270)),
    unimpl("process_vm_writev", ArchNrs(x86_64=311, arm64=271, rv64=271)),
    unimpl("kcmp", ArchNrs(x86_64=312, arm64=272, rv64=272)),
    unimpl("finit_module", ArchNrs(x86_64=313, arm64=273, rv64=273)),
    unimpl("sched_setattr", ArchNrs(x86_64=314, arm64=274, rv64=274)),
    unimpl("sched_getattr", ArchNrs(x86_64=315, arm64=275, rv64=275)),
    impl("renameat2", ["int olddirfd", "char* oldpath", "int newdirfd", "char* newpath", "int flags"], ArchNrs(x86_64=316, arm64=276, rv64=276)),
    unimpl("seccomp", ArchNrs(x86_64=317, arm64=277, rv64=277)),
    impl("getrandom", ["void* buf", "size_t buflen", "int flags"], ArchNrs(x86_64=318, arm64=278, rv64=278)),
    unimpl("memfd_create", ArchNrs(x86_64=319, arm64=279, rv64=279)),
    unimpl("kexec_file_load", ArchNrs(x86_64=320, arm64=294, rv64=294)),
    unimpl("bpf", ArchNrs(x86_64=321, arm64=280, rv64=280)),
    unimpl("execveat", ArchNrs(x86_64=322, arm64=281, rv64=281)),
    unimpl("userfaultfd", ArchNrs(x86_64=323, arm64=282, rv64=282)),
    unimpl("membarrier", ArchNrs(x86_64=324, arm64=283, rv64=283)),
    unimpl("mlock2", ArchNrs(x86_64=325, arm64=284, rv64=284)),
    unimpl("copy_file_range", ArchNrs(x86_64=326, arm64=285, rv64=285)),
    unimpl("preadv2", ArchNrs(x86_64=327, arm64=286, rv64=286)),
    unimpl("pwritev2", ArchNrs(x86_64=328, arm64=287, rv64=287)),
    unimpl("pkey_mprotect", ArchNrs(x86_64=329, arm64=288, rv64=288)),
    unimpl("pkey_alloc", ArchNrs(x86_64=330, arm64=289, rv64=289)),
    unimpl("pkey_free", ArchNrs(x86_64=331, arm64=290, rv64=290)),
    impl("statx", ["int dirfd", "char* pathname", "int flags", "unsigned int mask", "struct statx* statxbuf"], ArchNrs(x86_64=332, arm64=291, rv64=291)),
    unimpl("io_pgetevents", ArchNrs(x86_64=333, arm64=292, rv64=292)),
    unimpl("rseq", ArchNrs(x86_64=334, arm64=293, rv64=293)),
    unimpl("pidfd_send_signal", ArchNrs(x86_64=424, arm64=424, rv64=424)),
    unimpl("io_uring_setup", ArchNrs(x86_64=425, arm64=425, rv64=425)),
    unimpl("io_uring_enter", ArchNrs(x86_64=426, arm64=426, rv64=426)),
    unimpl("io_uring_register", ArchNrs(x86_64=427, arm64=427, rv64=427)),
    unimpl("open_tree", ArchNrs(x86_64=428, arm64=428, rv64=428)),
    unimpl("move_mount", ArchNrs(x86_64=429, arm64=429, rv64=429)),
    unimpl("fsopen", ArchNrs(x86_64=430, arm64=430, rv64=430)),
    unimpl("fsconfig", ArchNrs(x86_64=431, arm64=431, rv64=431)),
    unimpl("fsmount", ArchNrs(x86_64=432, arm64=432, rv64=432)),
    unimpl("fspick", ArchNrs(x86_64=433, arm64=433, rv64=433)),
    unimpl("pidfd_open", ArchNrs(x86_64=434, arm64=434, rv64=434)),
    unimpl("clone3", ArchNrs(x86_64=435, arm64=435, rv64=435)),
    unimpl("close_range", ArchNrs(x86_64=436, arm64=436, rv64=436)),
    unimpl("openat2", ArchNrs(x86_64=437, arm64=437, rv64=437)),
    unimpl("pidfd_getfd", ArchNrs(x86_64=438, arm64=438, rv64=438)),
    impl("faccessat2", ["int dirfd", "char* pathname", "int mode", "int flags"], ArchNrs(x86_64=439, arm64=439, rv64=439)),
    unimpl("process_madvise", ArchNrs(x86_64=440, arm64=440, rv64=440)),
    unimpl("epoll_pwait2", ArchNrs(x86_64=441, arm64=441, rv64=441)),
    unimpl("mount_setattr", ArchNrs(x86_64=442, arm64=442, rv64=442)),
    unimpl("quotactl_fd", ArchNrs(x86_64=443, arm64=443, rv64=443)),
    unimpl("landlock_create_ruleset", ArchNrs(x86_64=444, arm64=444, rv64=444)),
    unimpl("landlock_add_rule", ArchNrs(x86_64=445, arm64=445, rv64=445)),
    unimpl("landlock_restrict_self", ArchNrs(x86_64=446, arm64=446, rv64=446)),
    unimpl("memfd_secret", ArchNrs(x86_64=447, arm64=447, rv64=447)),
    unimpl("process_mrelease", ArchNrs(x86_64=448, arm64=448, rv64=448)),
    unimpl("futex_waitv", ArchNrs(x86_64=449, arm64=449, rv64=449)),
    unimpl("set_mempolicy_home_node", ArchNrs(x86_64=450, arm64=450, rv64=450)),
]

SYSCALLS: Dict[str, Syscall] = {s.name: s for s in _syscall_list}
assert len(SYSCALLS) == len(_syscall_list), "Duplicate entries for same syscall detected"

# -----------------------------------------------------------------------------
# Auxilary Syscalls
# These are WALI imports needed for runtime but not part of Linux syscall set
# -----------------------------------------------------------------------------
@dataclass
class AuxSyscall:
    name: str
    args: List[SyscallArg]
    args_id: List[str]
    result: SyscallArg = None

def impl_aux(name: str, args: List[SyscallArg], result: SyscallArg) -> AuxSyscall:
    """Full creation helper for fully defined (implemented) auxiliary syscalls."""
    args_ty, args_id = map(list, zip(*[split_args(a) for a in args])) if args else ([], [])
    return AuxSyscall(name=name, args=args_ty, args_id=args_id, result=result)

_aux_syscall_list: List[Syscall] = [
    # Startup, Args, Env
    impl_aux("__init", [], "int"),
    impl_aux("__deinit", [], "int"),
    impl_aux("__proc_exit", ["int status"], None),
    impl_aux("__cl_get_argc", [], "unsigned int"),
    impl_aux("__cl_get_argv_len", ["unsigned int arg_index"], "unsigned int"),
    impl_aux("__cl_copy_argv", ["char* argbuf", "unsigned int arg_index"], "int"),
    impl_aux("__get_init_envfile", ["char* pathbuf", "unsigned int bufsize"], "int"),
    # Threads
    impl_aux("__wasm_thread_spawn", ["fn(int, void*) wasm_start_fn", "void* args"], "int"),
    # Set/longjmps
    impl_aux("sigsetjmp", ["void* sigjmp_buf", "int savesigs"], "int"),
    impl_aux("longjmp", ["void* env", "int val"], None),
    impl_aux("setjmp", ["void* env"], "int"),
]

AUX_SYSCALLS: Dict[str, AuxSyscall] = {s.name: s for s in _aux_syscall_list}
assert len(AUX_SYSCALLS) == len(_aux_syscall_list), "Duplicate entries for same aux syscall detected"


# Utility functions for exporting syscall data
def dump_syscalls_to_csv():
    import csv
    
    header = [
        "NR", "Syscall", "# Args", 
        "a1", "a2", "a3", "a4", "a5", "a6", 
        "Aliases", "Unused Aliases", 
        "aarch64_NR", "riscv64_NR"
    ]
    
    filename = "csvs/syscall_full_format.csv"

    with open(filename, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(header)

        for s in _syscall_list:
            # Prepare args (pad to 6)
            padded_args = s.args + [""] * (6 - len(s.args))
            
            nr_x86 = s.nrs.x86_64
            nr_arm = s.nrs.arm64
            nr_rv = s.nrs.rv64
            
            # Joins
            aliases_str = ",".join(s.aliases)
            unused_aliases_str = ",".join(s.unused_aliases)

            row = [
                nr_x86,
                s.name,
                len(s.args) if s.implemented else "",
                *padded_args,
                aliases_str,
                unused_aliases_str,
                nr_arm,
                nr_rv
            ]
            writer.writerow(row)

# Simple main for check validity of spec
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Process syscall definitions.")
    parser.add_argument("-d", "--dump", action="store_true", help="Dump syscalls to CSV (csvs/syscall_full_format.csv)")
    args = parser.parse_args()
    if args.dump:
        dump_syscalls_to_csv()
        print(f"Wrote {len(_syscall_list)} definitions to csvs/syscall_full_format.csv")
    else:
        num_implemented = len([x for x in SYSCALLS.values() if x.implemented])
        print(f"Loaded spec successfully; {num_implemented} implemented Linux syscalls ({len(SYSCALLS)} defined) + {len(AUX_SYSCALLS)} auxiliary calls.")
