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
        if let Err(e) = prompt() {
            eprintln!("{}", e);
            panic!();
        }

        let line = match read_in() {
            Some(line) => line,
            None => {
                return Ok(());
            }
        };

        let mut cmds = line.trim().split("|").peekable(); //  "|" splits commands and make cmds peekable
        let mut previous_command = None;
        let mut pipe_in: Stdio;
        let mut pipe_out: Stdio;

        let mut previous_pipe = false;

        while let Some(cmd) = cmds.next() {
            let mut args = cmd.trim().split_whitespace().peekable(); //  in cmd split with whitespace
            let prog = match args.next() {
                //  executable program
                None => {
                    //  nothing happened in the command
                    continue;
                }
                Some(prog) => prog,
            };
            let mut real_args: Vec<&str> = Vec::new();

            while let Some(&arg) = args.peek() {
                //  form real args
                if arg != ">>" && arg != ">" && arg != "<" {
                    real_args.push(arg);
                    args.next();
                } else {
                    break;
                }
            }

            'arg: loop {
                pipe_in = if previous_pipe {
                    previous_command.map_or(Stdio::inherit(), |former: Child| {
                        Stdio::from(former.stdout.unwrap())
                    })
                } else {
                    Stdio::inherit()
                };
                pipe_out = Stdio::inherit();
                match args.peek() {
                    //  no >, >>, < redirection
                    None => {
                        pipe_out = if cmds.peek().is_some() {
                            previous_pipe = true;
                            Stdio::piped()
                        } else {
                            Stdio::inherit()
                        };
                    }
                    Some(&arg) => {
                        if arg == "<" {
                            if !previous_pipe {
                                args.next(); //  skip >, >>, <
                                pipe_in = unsafe {
                                    Stdio::from_raw_fd(
                                        File::open(args.next().unwrap()).unwrap().into_raw_fd(),
                                    )
                                }
                            } else {
                                previous_pipe = false;
                            }
                        } else if arg == ">>" {
                            pipe_out = unsafe {
                                let output_file = args.next().unwrap();
                                Stdio::from_raw_fd(
                                    OpenOptions::new()
                                        .append(true)
                                        .open(output_file)
                                        .unwrap()
                                        .into_raw_fd(),
                                )
                            }
                        } else if arg == ">" {
                            pipe_out = unsafe {
                                let output_file = args.next().unwrap();
                                Stdio::from_raw_fd(
                                    OpenOptions::new()
                                        .write(true)
                                        .open(output_file)
                                        .unwrap()
                                        .into_raw_fd(),
                                )
                            }
                        }
                    }
                }
                previous_command = None;
                match prog {
                    "cd" => {
                        cd(&real_args)?;
                    }
                    "echo" => {
                        previous_command = Some(match echo(pipe_in, pipe_out, &mut real_args) {
                            Ok(out) => out,
                            Err(e) => {
                                eprintln!("{}", e);
                                continue;
                            }
                        });
                    }
                    "exit" => {
                        //  same as "Ctrl D"
                        println!();
                        return Ok(());
                    }
                    "export" => {
                        export(&real_args);
                    }
                    _ => {
                        match Command::new(prog)
                            .args(&real_args)
                            .stdin(pipe_in)
                            .stdout(pipe_out)
                            .spawn()
                        {
                            Ok(out) => {
                                previous_command = Some(out);
                            }
                            Err(e) => {
                                eprintln!("{}", e);
                                continue;
                            }
                        }
                    }
                }
                if let None = args.peek() {
                    break 'arg;
                }
            }
        }
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
fn path_interpret(origin: &str) -> (String, bool) {
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

    if goal.contains("/dev/fd/") {}

    (goal, special)
}

fn variable_interpret(origin: &str) -> String {
    let mut goal = String::new();

    if let Ok(value) = env::var(origin) {
        goal = value;
    }
    goal
}

fn read_in() -> Option<String> {
    let mut line = String::new();
    stdin().read_line(&mut line).expect("readin error");
    if line == "" {
        //  "Ctrl D" makes the line a empty string
        println!(); //  print new line to have a better experience and eliminate bad prompts
        return None;
    }
    Some(line)
}

fn cd(real_args: &Vec<&str>) -> Result<(), VarError> {
    let (dir, _) = path_interpret(if real_args.len() == 0 {
        "~"
    } else {
        &real_args[0]
    });
    match env::set_current_dir(dir) {
        //  change the directory as dir defined
        Err(message) => {
            println!("{}", message);
        }
        _ => (),
    }
    Ok(())
}

fn echo(pipe_in: Stdio, pipe_out: Stdio, args: &mut Vec<&str>) -> Result<Child, std::io::Error> {
    let mut new_args: Vec<String> = Vec::new();
    for i in 0..args.len() {
        if args[i].starts_with("$") {
            new_args.push(variable_interpret((args[i].get(1..args[i].len())).unwrap()))
        }
        else {
            new_args.push(String::from(args[i]));
        }
    }
    Command::new("echo")
        .stdin(pipe_in)
        .stdout(pipe_out)
        .args(new_args)
        .spawn()
}

fn export(real_args: &Vec<&str>) {
    for i in 0..real_args.len() {
        let mut assign = real_args[i].split('=');
        let name = assign.next().expect("No variable name");
        let value = assign.next().expect("No variable value");
        env::set_var(name, value);
    }
}
