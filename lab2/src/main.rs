extern crate ctrlc;
extern crate libc;
extern crate nix;
use libc::{kill, pid_t, SIGINT};
use std::cell::RefCell;
use std::env::{self, VarError};
use std::error::Error;
use std::fs::{File, OpenOptions};
use std::io::{stdin, stdout, Write};
use std::os::unix::io::{FromRawFd, IntoRawFd};
use std::process::{Child, Command, Stdio};
use whoami;

fn main() -> Result<(), Box<dyn Error>> {
    ctrlc::set_handler(move || {
        kill_exec();
        println!();
    })?;
    loop {
        match prompt() {
            Ok(()) => (),
            _ => (),
        }

        let mut line = String::new();
        stdin().read_line(&mut line)?;
        if line == "" {
            //  "Ctrl D" makes the line a empty string
            println!(); //  print new line to have a better experience and eliminate bad prompts
            return Ok(());
        }
        let mut cmds = line.trim().split("|").peekable(); //  pipes split commands
        let mut previous_command = None;

        while let Some(cmd) = cmds.next() {
            let mut out_redirect: bool = false;
            let mut out_append_redirect: bool = false;
            let mut in_redirect: bool = false;

            //  check if there is file redirection
            if cmd.contains(" >> ") {
                out_append_redirect = true;
            } else if cmd.contains(" > ") {
                out_redirect = true;
            } else if cmd.contains(" < ") {
                in_redirect = true;
            }
            let mut args = cmd.trim().split_whitespace().peekable(); //  in cmd split with whitespace
            let cmd = match args.next() {
                None => {
                    //  nothing happened in the command
                    continue;
                }
                Some(cmd) => cmd,
            };
            let mut former_args: Vec<&str> = Vec::new();
            let mut latter_file = String::new();
            loop {
                match args.next() {
                    Some(arg) => {
                    if arg != ">>" && arg != ">" && arg != "<" {
                        former_args.push(&arg);
                    } else {
                        break;
                    }}
                    None => {
                        break;
                    }
                }
            }
            if out_redirect || out_append_redirect || in_redirect {
                match args.next() {
                    Some(file) => {
                        latter_file = file.to_string();
                    }
                    None => {
                        println!("error occurs when reading args.");
                        continue;
                    }
                }
            }
            for i in 0..former_args.len() {
                let args = former_args[i];
                if args.starts_with("'") || args.starts_with("\"") {
                    former_args[i] = former_args[i].trim_start();
                }
                else if args.ends_with("'") || args.ends_with("\"") {
                    former_args[i] = former_args[i].trim_end();
                }
            }
            match cmd {
                "exit" => {
                    //  same as "Ctrl D"
                    println!();
                    return Ok(());
                }
                "cd" => {
                    let (dir, _) = path_interpret(former_args[0]);
                    match env::set_current_dir(dir) {
                        //  change the directory as dir defined
                        Err(message) => {
                            println!("{}", message);
                        }
                        _ => (),
                    }
                }
                "export" => {
                    //  set env variable
                    for arg in former_args {
                        let mut assign = arg.split('=');
                        let name = assign.next().expect("No variable name");
                        let value = assign.next().expect("No variable value");
                        env::set_var(name, value);
                    }
                }
                _ => {
                    let special = false;
                    if out_append_redirect || out_redirect || in_redirect { //  TCP redirection

                    }

                    let former_args = former_args.iter();
                    //  if there is former program has output, then use its stdout as future stdin, else inherit the shell's stdin as future stdin
                    let pipe_in = if in_redirect {
                        unsafe {
                            Stdio::from_raw_fd(File::open(&latter_file).unwrap().into_raw_fd())
                        }
                    } else {
                        previous_command.map_or(Stdio::inherit(), |former: Child| {
                            Stdio::from(former.stdout.unwrap())
                        })
                    };

                    //  if there is command following this command, then pipe the output, else inherit the stdout
                    let pipe_out = if out_redirect {
                        unsafe {
                            Stdio::from_raw_fd(File::create(&latter_file).unwrap().into_raw_fd())
                        }
                    } else if out_append_redirect {
                        unsafe {
                            Stdio::from_raw_fd(
                                OpenOptions::new()
                                    .append(true)
                                    .open(&latter_file)
                                    .unwrap()
                                    .into_raw_fd(),
                            )
                        }
                    } else if cmds.peek().is_some() {
                        Stdio::piped()
                    } else {
                        Stdio::inherit()
                    };

                    //  previous_command has been moved in "let stdin = ..."
                    previous_command = None;
                    let output = Command::new(cmd)
                        .args(former_args)
                        .stdin(pipe_in)
                        .stdout(pipe_out)
                        .spawn(); //  as a child process
                    match output {
                        Ok(out) => {
                            previous_command = Some(out);
                        }
                        Err(e) => {
                            eprintln!("{}", e); //  print error message but not panic
                        }
                    }
                }
            };
        }
        //  wait for the last command to finish its execution, or the shell will be messed
        if let Some(mut final_command) = previous_command {
            final_command.wait()?;
        }
    }
}

fn home_path() -> Result<String, VarError> {
    match env::var("HOME") {
        Ok(dist) => Ok(dist.to_string()),
        Err(e) => {
            println!("{}", e);
            Err(e)
        }
    }
}

fn prompt() -> std::io::Result<()> {
    println!(
        "{}: {}",
        whoami::username(),
        env::current_dir()?
            .to_str()
            .expect("Getting current dir failed"),
    );
    print!(
        "{} ",
        match whoami::username().as_str() {
            "root" => "#",
            _ => "$",
        }
    );
    stdout().flush().unwrap();
    Ok(())
}

thread_local! {
    static EXEC_PIDS: RefCell<Vec<u32>> = RefCell::new(Vec::new());
}

fn kill_exec() {
    EXEC_PIDS.with(|c| {
        for pid in c.borrow().iter() {
            unsafe {
                kill(*pid as pid_t, SIGINT);
            }
        }
    });
}

//  解释路径，如果含特殊地址还需要再解析
fn path_interpret(origin: &str) -> (String , bool) {
    let mut goal: String = String::new();
    let special = false;

    let origin = origin.split("/");
    for item in origin {
        match item {
            "" => {
                goal.push_str("/");
            }
            "~" => {
                goal.push_str(&home_path().unwrap());
                goal.push_str("/");
            }
            _ => {
                goal.push_str(item);
            }
        }
    }

    if goal.contains("/dev/fd/") {

    }

    (goal, special)
}