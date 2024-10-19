# OS-Shell-Project
This project is a custom shell developed as part of my Operating Systems class. It simulates the basic functionalities of a Unix-like shell, supporting command execution, pipes, input/output redirection, and background processes. The shell also includes built-in commands like cd, exit, and help.


## Features

1. Command Execution
The shell allows you to run typical shell commands like:
bash
ls
cat
echo "Hello World"
```

### 2. Pipes
You can chain multiple commands using pipes (`|`) to connect their input and output:
```bash
cat file.txt | wc
```

### 3. Input and Output Redirection
Redirect input and output using `<` and `>`:
```bash
wc < input.txt > output.txt
```

### 4. Background Processes
You can run commands in the background using `&`:
```bash
sleep 10 &
```

### 5. Built-in Commands
The shell includes a few built-in commands such as:
- `cd <path>`: Change the current directory.
- `exit`: Exit the shell.
- `help`: Display available built-in commands.

## How to Compile and Run

1. **Clone the repository**:
   ```bash
   git clone https://github.com/BaileyYi19307/OS-Shell-Project.git
   ```

2. **Navigate to the project directory**:
   ```bash
   cd OS-Shell-Project
   ```

3. **Compile the shell program**:
   ```bash
   gcc basicShell.c -o basicShell
   ```

4. **Run the shell**:
   ```bash
   ./basicShell
   ```

## Example Commands

### Execute a Command
```bash
ls -l
```

### Use Pipes and Redirection
```bash
cat input.txt | grep "keyword" > output.txt
```

### Run a Background Process
```bash
sleep 10 &
```

## License

[Specify the license here, if applicable, or say the project is for educational purposes.]

---

This shell is a simplified version of a Unix-like shell designed for educational purposes as part of an Operating Systems class. It demonstrates core concepts such as process management, piping, and I/O redirection.
```

### Breakdown of What to Include:
- **Introduction**: Briefly explain what the project is.
- **Features**: List the key functionalities of your shell.
- **How to Compile and Run**: Step-by-step instructions for someone who wants to run the shell.
- **Example Commands**: Provide examples of how to use the shell (pipes, redirection, etc.).
- **License**: Specify a license or mention that the project was developed for educational purposes.

Feel free to adjust the details as needed. This `README.md` will make your project easy to understand for anyone viewing it on GitHub! Let me know if you'd like to customize it further.
