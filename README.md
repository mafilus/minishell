# Mini Shell
Mini shell is a shell using readline and the libc. It's a minimal implementation of a shell.
----
## Compilation
Install dependencies:
```bash
apt install build-essential libreadline-dev || pacman -S base-devel readline  
```

Compilation:

```bash
make
```
----
## Usage

* Run the shell
```bash
./minishell
```

* Run a command
```bash
./minishell -c 'ls /'
```

* Run a script
```bash
./minishell -s script.minishell
```
