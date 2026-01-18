linux-syscall-toolkit

A custom syscall and system information toolkit for Linux, with optional macOS analogs.

This project demonstrates custom Linux syscalls alongside cross-platform macOS equivalents. Built as a hands-on systems programming portfolio piece, it bridges kernel development and user-space system inspection.
----------------------
Purpose
This toolkit explores practical system introspection across platforms by pairing custom Linux kernel syscalls with macOS user-space tools. The goal is to provide a lightweight, consistent interface for inspecting process behavior and user session state.

Key goals:

* Hands-on Linux syscall development

* End-to-end syscall wiring and user-space testing

* Platform-aware tooling for both Linux and macOS

* A consistent interface to system information for debugging and monitoring

From retrieving child processes of a parent PID to inspecting the currently logged-in user's session, this toolkit shows how OS-specific tooling can still share a common systems language. User identity, session scope, and process state remain foundational to low-level systems insight.
----------------------------
Custom Linux Syscalls
This project implements four syscalls in a custom-built Linux kernel:

1. aaron_tuck (syscall 450)

  * A basic syscall that returns 0

  * Used to validate syscall wiring and kernel rebuilds

2. track_process_info (syscall 451)
Retrieves process information for a given PID:

  * PID and PPID

  * UID

  * Command name

  * Process state (running, sleeping, zombie)

3. get_children_pids (syscall 452)

  * Returns a list of child PIDs for a given parent PID

  * Capped at 128 child processes

4. user_session_info (syscall 453)
Returns user and session details for the current user:

  * UID and GID

  * Username

  * Home directory

  * Default shell

Each syscall was added to:

  * syscalls.h

  * syscall_64.tbl

  * A dedicated .c file in the kernel source

  * A shared header: include/linux/track_info.h

Kernel rebuilt cleanly and boots as: aarontuck_v2
---------------------------------
**Toolkit Structure (Linux)**

```text
hackKernel/
├── syscall-export/                # Exported kernel syscall source files
│   ├── sys_aaron_tuck.c
│   ├── sys_track_process.c
│   ├── sys_get_children_pids.c
│   ├── sys_user_session_info.c
├── syscall-tests/                 # User-space syscall test programs
│   ├── track_test.c
│   ├── children_test.c
│   ├── user_session_test.c
└── include/linux/track_info.h     # Shared struct definitions
```


---------------------------------
Build Instructions (Linux)
From your kernel source directory:
```
cd ~/my-kernel
make -j$(nproc)
sudo make modules_install
sudo make install
```

Reboot into the new kernel (aarontuck_v2).

Compile the user-space test programs:
```
gcc -o track_test track_test.c
gcc -o children_test children_test.c
gcc -o user_session_test user_session_test.c
```

macOS Companion Tools (Optional)
These user-space tools mimic the Linux syscall behavior using macOS system APIs.

1. proc_info.c

* Uses sysctl() to retrieve PID info

* Mirrors track_process_info

2. user_session.c

* Uses getuid(), getpwuid(), and getenv() to retrieve:

* UID and username

* Home directory

* Shell

* Mirrors user_session_info
--------------------------------
Project layout:
```
macProcInfoTool/
├── proc_info.c
├── user_session.c
```

Compile with:
```
clang proc_info.c -o proc_info
clang user_session.c -o user_session

```
Example Usage (Linux)
```
./track_test 1234
./children_test 1
./user_session_test
```

Example Usage (macOS)
```
./proc_info 1234
./user_session
```

Future Enhancements

Add a logging kernel module

Track file descriptors or open files per PID

Expand session reporting (groups, login type, and related metadata)
