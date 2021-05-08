use std::env::{self, VarError};
use std::error::Error;
use std::fs::File;
use std::io::{self, stderr, stdin, stdout, BufRead, Write};
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
        if line == "" {     //  "Ctrl D" makes the line a empty string
            println!();     //  print new line to have a better experience and eliminate bad prompts
            return Ok(());
        }
        let mut cmds = line.trim().split(" | ").peekable(); //  pipes split commands
        let mut previous_command = None;

        while let Some(cmd) = cmds.next() {
            let mut args = cmd.trim().split_ascii_whitespace(); //  in cmd split with whitespace
            let cmd = match args.next() {
                None => {                       //  nothing happened in the command
                    continue;
                }
                Some(cmd) => cmd,
            };
            match cmd {
                "exit" => {     //  same as "Ctrl D"
                    println!();
                    return Ok(());
                }
                _ => {
                    //  if there is former program has output, then use its stdout as future stdin, else inherit the shell's stdin as future stdin
                    let stdin = previous_command.map_or(Stdio::inherit(), |former: Child| {
                        Stdio::from(former.stdout.unwrap())
                    });

                    //  if there is command following this command, then pipe the output, else inherit the stdout
                    let stdout = if cmds.peek().is_some() {
                        Stdio::piped()
                    } else {
                        Stdio::inherit()
                    };

                    //  previous_command has been moved in "let stdin = ..."
                    previous_command = None;
                    match cmd {
                        "cd" => {
                            let dir = match args.next() {
                                None => match home_path() { //  "cd\n"
                                    Err(_e) => {
                                        previous_command = None;
                                        continue;
                                    }
                                    Ok(dir) => dir,
                                },
                                Some(dist) => {             //  "cd /path"
                                    if dist == "~" {        //  "cd ~"
                                        match home_path() {
                                            Err(_e) => {
                                                previous_command = None;
                                                continue;
                                            }
                                            Ok(dir) => dir,
                                        }
                                    } else {
                                        dist.to_string()
                                    }
                                }
                            };
                            match env::set_current_dir(dir) {   //  change the directory as dir defined
                                Err(message) => {
                                    println!("{}", message);
                                }
                                _ => (),
                            }
                        }
                        // "pwd" => {
                        //     let err = "Getting current dir failed";
                        //     println!("{}", env::current_dir().expect(err).to_str().expect(err));
                        // }
                        "export" => {   //  set env variable
                            for arg in args {
                                let mut assign = arg.split('=');
                                let name = assign.next().expect("No variable name");
                                let value = assign.next().expect("No variable value");
                                env::set_var(name, value);
                            }
                        }
                        // "env" => {
                        //     for (key, value) in env::vars() {
                        //         println!("{}={}", key, value);
                        //     }
                        // }
                        _ => {
                            let output = Command::new(cmd)
                                .args(args)
                                .stdin(stdin)
                                .stdout(stdout)
                                .spawn();   //  as a child process
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
    print!(
        "{}: {}$ ",
        whoami::realname(),
        env::current_dir()?
            .to_str()
            .expect("Getting current dir failed")
    );
    io::stdout().flush().unwrap();
    Ok(())
}
