use std::{ffi::OsString, path::PathBuf};

use crate::Source;

impl Source {
    /// Produce a storage location for the this source while potentially querying environment variables using `env_var(<name>)`,
    /// or `None` if the storage location could not be obtained.
    ///
    /// Note that local sources are returned as relative paths to be joined with the base in a separate step.
    pub fn storage_location(self, env_var: &mut dyn FnMut(&str) -> Option<OsString>) -> Option<PathBuf> {
        use Source::*;
        Some(match self {
            GitInstallation => gix_path::env::installation_config_prefix()?.join("gitattributes"),
            System => {
                if env_var("GIT_ATTR_NOSYSTEM").is_some() {
                    return None;
                } else {
                    let prefix = gix_path::env::system_prefix()?;
                    // Git for Windows builds ETC_GITATTRIBUTES as ../etc/gitattributes.
                    let prefix = if cfg!(windows) {
                        prefix.parent().unwrap_or(prefix)
                    } else {
                        prefix
                    };
                    prefix.join("etc/gitattributes")
                }
            }
            Git => return gix_path::env::xdg_config("attributes", env_var),
            Local => PathBuf::from("info/attributes"),
        })
    }
}
