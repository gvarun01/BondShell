# BondShell - A Simple Interactive Shell

BondShell is a custom interactive shell implemented in C. It provides a range of built-in commands for navigation, file system operations, process management, and command history, alongside support for executing system commands.

## Features

*   **Command Execution:** Supports execution of system commands both in foreground and background.
*   **Built-in Commands:**
    *   `cd <path>`: Change current directory.
    *   `hop <path1> [<path2> ...]`: Change directory, supporting multiple hops and special characters like `~`, `.`, `..`, `-`. Prints the new path after each hop.
    *   `reveal [-a] [-l] [<path>]`: List files and directories (similar to `ls`). Supports `-a` (all) and `-l` (long format).
    *   `seek [-d|-f] [-e] <target> [<directory>]`: Search for files or directories.
        *   `-d`: Search for directories only.
        *   `-f`: Search for files only.
        *   `-e`: If a single match is found, execute it (changes to directory if it's a dir, prints content if it's a file).
    *   `proclore [<pid>]`: Display process information (PID, status, group, memory, path). Defaults to the shell's PID if no argument.
    *   `activities`: Display currently running background processes initiated by the shell.
    *   `log`: Display command history (last 15 commands).
        *   `log purge`: Clear command history.
        *   `log execute <index>`: Execute a command from history by its index.
    *   `iman <command>`: Fetch and display the man page for a command from an online source (man.he.net).
    *   `neonate -n <seconds>`: Periodically print the PID of the most recently created process every `<seconds>`. Press 'x' to exit.
    *   `ping <pid> <signal_number>`: Send a specified signal to a process.
    *   `fg <pid>`: Bring a background job to the foreground.
    *   `bg <pid>`: Resume a stopped background job in the background.
    *   `help`: Display a help message listing available commands.
    *   `exit`: Terminate the BondShell.
*   **I/O Redirection:** Supports input (`<`), output (`>`), and append output (`>>`) redirection.
*   **Piping:** Supports piping commands (`|`).
*   **Signal Handling:**
    *   `Ctrl+C`: Send SIGINT to the current foreground process.
    *   `Ctrl+D`: Log out (exit shell).
    *   `Ctrl+Z`: Send SIGTSTP to the current foreground process and move it to the background.
*   **Aliases and Functions:** Supports defining aliases and simple functions in a `.myshrc` file located in the shell's startup directory.
    *   Aliases: `alias ll='ls -l'`
    *   Functions: Simple multi-line functions with argument `$1`.
*   **Prompt:** Displays username, system name, and current working path (relative to home as `~`). Shows execution time of the last command if it exceeded 2 seconds.
*   **Startup Animation:** A short animation on shell startup.

## Building the Shell

A Makefile is provided for easy compilation.

1.  **Prerequisites:**
    *   A C compiler (e.g., GCC)
    *   Make

2.  **Compilation:**
    *   To build the release version:
        ```bash
        make all
        # or simply
        make
        ```
    *   To build a debug version (with debugging symbols):
        ```bash
        make debug
        ```
    The compiled executable `bondshell` will be located in the `build/` directory.

3.  **Cleaning Build Files:**
    ```bash
    make clean
    ```

## Running BondShell

After successful compilation, run the shell from the project root directory:

```bash
./build/bondshell
```
Or, use the Makefile target:
```bash
make run
```

Upon startup, BondShell will look for a `.myshrc` file in its initial working directory to load aliases and functions.

## Known Issues / Limitations during Refactoring

*   During the refactoring process, persistent compilation errors were encountered related to POSIX symbols like `usleep`, `setpgrp`, and `SIGKILL` in the provided build environment. These issues could not be fully resolved despite various standard approaches (feature test macros, include order adjustments). The Makefile and code are structured to work in a standard POSIX environment, but the shell might not compile or run correctly in the testing environment due to these unresolved symbol issues.

---
Crafted with ❤️ by Varun Gupta. (Original author)
