# Linux System Call Toolkit

Three custom Linux system calls, added to a rebuilt kernel, that expose process hierarchy and user-session information to user space, paired with a cross-platform macOS equivalent built entirely in user space. The focus is correctness at the kernel boundary: race-free task lookups and safe data transfer between kernel and user space.

> Educational systems-programming project. The syscalls run in a custom-built kernel; do not add unreviewed syscalls to a production system.

---

## Why it's interesting

The point of this project isn't that it retrieves a PID. It's *how* it does it safely inside the kernel, where a careless lookup can dereference a task that another CPU is tearing down.

- **Race-free task lookup.** Tasks are located with `find_task_by_vpid` inside an `rcu_read_lock()` critical section, so the process table can't shift under the lookup.
- **Reference counting across the lock boundary.** For the syscall that needs to read task fields after the lookup, the code takes a reference with `get_task_struct()` *before* releasing the RCU lock, works with the task, then releases it with `put_task_struct()`. This is what keeps the task alive even if it exits mid-syscall, the correct pattern for touching a `task_struct` outside RCU.
- **Safe kernel/user-space transfer.** Every syscall validates the user pointer with `copy_from_user()` on the way in and `copy_to_user()` on the way out, returning `-EFAULT` rather than trusting user-supplied addresses.
- **Bounded output.** The child-PID enumeration is capped at 128 entries so a process with a huge child set can't overrun the fixed user buffer.

---

## The system calls

Implemented in a custom-built kernel (`aarontuck_v2`). Three do real work; one is a wiring test.

**`track_process_info`** — given a PID, returns its PPID, UID, command name, and process state. Uses the RCU-lookup-then-reference-count pattern described above so the fields are read from a task that can't be freed underneath it.

**`get_children_pids`** — given a parent PID, returns that parent's child PIDs (up to 128) by walking the children list under RCU.

**`user_session_info`** — returns the calling process's UID and GID from its credential struct (`current_cred()`).

**`aaron_tuck`** — a no-argument syscall that returns 0. Used to validate syscall-table wiring and confirm a clean kernel rebuild before the real work.

Each syscall was registered in `syscalls.h` and `syscall_64.tbl`, implemented in its own source file, and backed by shared struct definitions in `track_info.h`.

---

## macOS companion tools

The same information, retrieved without any kernel modification, to show how the two platforms expose process state differently:

- **`proc_info.c`** — mirrors `track_process_info` using the `sysctl` interface (`CTL_KERN`/`KERN_PROC`) to read PPID, UID, command, and state.
- **`user_session.c`** — resolves UID, username, home directory, and default shell via `getuid`, `getpwuid`, and `getenv`.

The Linux side reaches this data by adding kernel code; the macOS side reaches comparable data purely from user space. Same questions, two very different answers about where the boundary sits.

---

## Shared data structures

`track_info.h` is written to compile on both sides of the boundary, guarding kernel-only includes behind `__KERNEL__`:

```c
struct track_info      { pid_t pid, ppid; uid_t uid; char comm[16]; long state; };
struct children_info   { pid_t parent_pid; size_t num_children; pid_t children[128]; };
struct user_session_info { uid_t uid; gid_t gid; char username[32]; char home_dir[128]; char shell[64]; };
```

---

## Repository layout

```text
linux-syscall-toolkit/
├── kernel/                        # syscall implementations (added to kernel source)
│   ├── aaron_tuck.c               # wiring/proof-of-life syscall
│   ├── sys_track_process.c        # track_process_info
│   ├── sys_get_children_pids.c    # get_children_pids
│   └── sys_user_session_info.c    # user_session_info
├── syscall-tests/                 # user-space test programs
│   ├── track_test.c
│   ├── children_test.c
│   └── user_session_test.c
├── include:linux/track_info.h     # shared struct definitions
└── macProcInfoTool/               # user-space macOS equivalents
    ├── proc_info.c
    └── user_session.c
```

---

## Build and run

**Linux (custom kernel).** From the kernel source tree with the syscalls and `track_info.h` added:

```bash
make -j$(nproc)
sudo make modules_install && sudo make install
# reboot into the aarontuck_v2 kernel
```

Then compile and run the user-space tests:

```bash
gcc -o track_test track_test.c
gcc -o children_test children_test.c
gcc -o user_session_test user_session_test.c

./track_test 1234        # info for PID 1234
./children_test 1        # children of PID 1
./user_session_test      # current user's UID/GID
```

**macOS (no kernel changes).**

```bash
clang proc_info.c -o proc_info
clang user_session.c -o user_session

./proc_info 1234
./user_session
```

---

## Notes and limitations

- Child enumeration is capped at 128 PIDs by the fixed-size buffer; a larger set is silently truncated.
- `user_session_info` on Linux returns UID and GID; the richer username/home/shell resolution lives in the macOS tool, which has `getpwuid`/`getenv` available in user space.
- This modifies and rebuilds a kernel. It is a learning artifact, not something to run on a machine you care about.

## Future work

- A logging kernel module to record syscall invocations
- Per-PID open file-descriptor reporting
- Expanded session metadata (group membership, login type)
