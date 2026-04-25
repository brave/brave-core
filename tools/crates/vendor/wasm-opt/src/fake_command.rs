use std::ffi::{OsStr, OsString};
use std::io::Result;
use std::path::Path;
use std::process::Command as StdCommand;
use std::process::{Child, ExitStatus, Output, Stdio};

/// Like [`std::process::Command`] but args are iterable in old versions of Rust.
pub struct Command {
    inner: StdCommand,
    cached_args: Vec<OsString>,
}

impl Command {
    pub fn new<S: AsRef<OsStr>>(program: S) -> Command {
        Command {
            inner: StdCommand::new(program),
            cached_args: vec![],
        }
    }

    pub fn arg<S: AsRef<OsStr>>(&mut self, arg: S) -> &mut Command {
        self.cached_args.push(OsString::from(arg.as_ref()));
        self.inner.arg(arg);
        self
    }

    pub fn args<I, S>(&mut self, args: I) -> &mut Command
    where
        I: IntoIterator<Item = S>,
        S: AsRef<OsStr>,
    {
        let args: Vec<_> = args.into_iter().collect();
        self.cached_args
            .extend(args.iter().map(|arg| OsString::from(arg)));
        self.inner.args(args);
        self
    }

    pub fn env<K, V>(&mut self, key: K, val: V) -> &mut Command
    where
        K: AsRef<OsStr>,
        V: AsRef<OsStr>,
    {
        self.inner.env(key, val);
        self
    }

    pub fn envs<I, K, V>(&mut self, vars: I) -> &mut Command
    where
        I: IntoIterator<Item = (K, V)>,
        K: AsRef<OsStr>,
        V: AsRef<OsStr>,
    {
        self.inner.envs(vars);
        self
    }

    pub fn env_remove<K: AsRef<OsStr>>(&mut self, key: K) -> &mut Command {
        self.inner.env_remove(key);
        self
    }

    pub fn env_clear(&mut self) -> &mut Command {
        self.inner.env_clear();
        self
    }

    pub fn current_dir<P: AsRef<Path>>(&mut self, dir: P) -> &mut Command {
        self.inner.current_dir(dir);
        self
    }

    pub fn stdin<T: Into<Stdio>>(&mut self, cfg: T) -> &mut Command {
        self.inner.stdin(cfg);
        self
    }

    pub fn stdout<T: Into<Stdio>>(&mut self, cfg: T) -> &mut Command {
        self.inner.stdout(cfg);
        self
    }

    pub fn stderr<T: Into<Stdio>>(&mut self, cfg: T) -> &mut Command {
        self.inner.stderr(cfg);
        self
    }

    pub fn spawn(&mut self) -> Result<Child> {
        self.inner.spawn()
    }

    pub fn output(&mut self) -> Result<Output> {
        self.inner.output()
    }

    pub fn status(&mut self) -> Result<ExitStatus> {
        self.inner.status()
    }

    pub fn get_args(&self) -> CommandArgs<'_> {
        CommandArgs {
            inner: self
                .cached_args
                .iter()
                .map(&|arg: &OsString| arg.as_os_str()),
        }
    }
}

pub struct CommandArgs<'cmd> {
    inner: std::iter::Map<
        std::slice::Iter<'cmd, OsString>,
        &'cmd dyn for<'r> Fn(&'r OsString) -> &'r OsStr,
    >, // omg
}

impl<'cmd> Iterator for CommandArgs<'cmd> {
    type Item = &'cmd OsStr;

    fn next(&mut self) -> Option<&'cmd OsStr> {
        self.inner.next()
    }
}

impl std::fmt::Debug for Command {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        self.inner.fmt(f)
    }
}
