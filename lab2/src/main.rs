extern crate libc;
extern crate nix;
use nix::unistd::pipe;
use std::env::{self, VarError};
use std::error::Error;
use std::fs::{File, OpenOptions};
use std::io::{self, stderr, stdin, stdout, BufRead, Write};
use std::os::unix::io::{FromRawFd, IntoRawFd};
use std::process::{exit, Child, Command, Stdio};
use whoami;

fn main() -> Result<(), Box<dyn Error>> {
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
            if cmd.contains(">") {
                out_redirect = true;
            } else if cmd.contains(">>") {
                out_append_redirect = true;
            } else if cmd.contains("<") {
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
            for arg in args.next() {
                if arg != ">>" && arg != ">" && arg != "<" {
                    former_args.push(arg);
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
            let former_args = former_args.iter();
            match cmd {
                "exit" => {
                    //  same as "Ctrl D"
                    println!();
                    return Ok(());
                }
                "cd" => {
                    let dir = match args.next() {
                        None => match home_path() {
                            //  "cd\n"
                            Err(_e) => {
                                continue;
                            }
                            Ok(dir) => dir,
                        },
                        Some(dist) => {
                            //  "cd /path"
                            if dist == "~" {
                                //  "cd ~"
                                match home_path() {
                                    Err(_e) => {
                                        continue;
                                    }
                                    Ok(dir) => dir,
                                }
                            } else {
                                dist.to_string()
                            }
                        }
                    };
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
                    for arg in args {
                        let mut assign = arg.split('=');
                        let name = assign.next().expect("No variable name");
                        let value = assign.next().expect("No variable value");
                        env::set_var(name, value);
                    }
                }
                _ => {
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
