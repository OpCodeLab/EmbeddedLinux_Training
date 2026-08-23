use std::io::{self, Write};
use libc::{kill, pid_t, SIGUSR1};

fn main() {
    print!("Enter the target process ID: ");
    io::stdout().flush().unwrap();

    let mut input = String::new();
    io::stdin()
        .read_line(&mut input)
        .expect("Failed to read input");

    let pid: pid_t = match input.trim().parse() {
        Ok(p) => p,
        Err(_) => {
            eprintln!("Error: please enter a valid integer PID");
            return;
        }
    };

    let result = unsafe { kill(pid, SIGUSR1) };

    if result == -1 {
        let err = io::Error::last_os_error();
        eprintln!("Error sending signal: {}", err);
    } else {
        println!("Sent SIGUSR1 to process {}", pid);
    }
}