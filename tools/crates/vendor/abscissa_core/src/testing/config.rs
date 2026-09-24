//! Support for writing config files and using them in tests

use crate::fs::{self, File, OpenOptions};
use serde::Serialize;
use std::{
    env,
    ffi::OsStr,
    io::{self, Write},
    path::{Path, PathBuf},
};

/// Number of times to attempt to create a file before giving up
const FILE_CREATE_ATTEMPTS: usize = 1024;

/// Configuration file RAII guard which deletes it on completion
#[derive(Debug)]
pub struct ConfigFile {
    /// Path to the config file
    path: PathBuf,
}

impl ConfigFile {
    /// Create a config file by serializing it to the given location
    pub fn create<C>(app_name: &OsStr, config: &C) -> Self
    where
        C: Serialize,
    {
        let (path, mut file) = Self::open(app_name);

        let config_toml = toml::to_string_pretty(config)
            .unwrap_or_else(|e| panic!("error serializing config as TOML: {}", e))
            .into_bytes();

        file.write_all(&config_toml)
            .unwrap_or_else(|e| panic!("error writing config to {}: {}", path.display(), e));

        Self { path }
    }

    /// Get path to the configuration file
    pub fn path(&self) -> &Path {
        self.path.as_ref()
    }

    /// Create a temporary filename for the config
    fn open(app_name: &OsStr) -> (PathBuf, File) {
        // TODO: fully `OsString`-based path building
        let filename_prefix = app_name.to_string_lossy().to_string();

        for n in 0..FILE_CREATE_ATTEMPTS {
            let filename = format!("{}-{}.toml", &filename_prefix, n);
            let path = env::temp_dir().join(filename);

            match OpenOptions::new().write(true).create_new(true).open(&path) {
                Ok(file) => return (path, file),
                Err(e) => {
                    if e.kind() == io::ErrorKind::AlreadyExists {
                        continue;
                    } else {
                        panic!("couldn't create {}: {}", path.display(), e);
                    }
                }
            }
        }

        panic!(
            "couldn't create {}.toml after {} attempts!",
            filename_prefix, FILE_CREATE_ATTEMPTS
        )
    }
}

impl Drop for ConfigFile {
    fn drop(&mut self) {
        fs::remove_file(&self.path).unwrap_or_else(|e| {
            eprintln!("error removing {}: {}", self.path.display(), e);
        })
    }
}
