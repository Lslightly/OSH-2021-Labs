use std::io::{self, stdin, BufRead, Write};
use std::env::{self, VarError};
use std::process::{exit, Command};
use whoami;

fn main() -> ! {
    loop {
        match prompt() {
            Ok(()) => (),
            _ => (),
        }

        let mut cmd = String::new();
        for line_res in stdin().lock().lines() {
            let line = line_res.expect("Read a line from stdin failed");
            cmd = line;
            break;
        }
        let mut args = cmd.split_whitespace();
        let prog = args.next();

        match prog {
            None => (),
            Some(prog) => {
                match prog {
                    "cd" => {
                        let dir = match args.next() {
                            None => {
                                match home_path() {
                                    Err(_e) => continue,
                                    Ok(dir) => dir
                                }
                            }
                            Some(dist) => {
                                if dist == "~" {
                                    match home_path() {
                                        Err(_e) => continue,
                                        Ok(dir) => dir,
                                    }
                                } else {
                                    dist.to_string()
                                }
                            }
                        };
                        match env::set_current_dir(dir) {
                            Err(message) => {
                                println!("{}", message);
                            }
                            _ => (),
                        }
                    }
                    "pwd" => {
                        let err = "Getting current dir failed";
                        println!("{}", env::current_dir().expect(err).to_str().expect(err));
                    }
                    "export" => {
                        for arg in args {
                            let mut assign = arg.split('=');
                            let name = assign.next().expect("No variable name");
                            let value = assign.next().expect("No variable value");
                            env::set_var(name, value);
                        }
                    }
                    "exit" => {
                        exit(0);
                    }
                    "env" => {
                        for (key, value) in env::vars() {
                            println!("{}={}", key, value);
                        }
                    }
                    _ => {
                        match Command::new(prog).args(args).status() {
                            Ok(_e) => (),
                            Err(e) => {
                                println!("{}", e);
                            }
                        }
                    }
                }
            }
        }
    }
}

fn home_path() -> Result<String, VarError> {
    match env::var("HOME") {
        Ok(dist) => {
            Ok(dist.to_string())
        }
        Err(e) => {
            println!("{}", e);
            Err(e)
        }
    }
}

fn prompt() -> std::io::Result<()> {
    print!("{}: {} $ ", whoami::realname(),  env::current_dir()?.to_str().expect("Getting current dir failed"));
    io::stdout().flush().unwrap();
    Ok(())
}