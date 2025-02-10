//! Paths for resources used by the application.
//!
//! Implemented as a trait to support extensibility and customizability, but
//! with a a set of framework conventions.

pub use canonical_path::{CanonicalPath as AbsPath, CanonicalPathBuf as AbsPathBuf};

// Just in case anyone gets confused why `Path` is private
pub use std::path::{Path, PathBuf};

use crate::FrameworkError;

/// Name of the application's secrets directory
pub(crate) const SECRETS_DIR: &str = "secrets";

/// Path to the application's executable.
pub trait ExePath {
    /// Get the path to the application's executable
    fn exe(&self) -> &AbsPath;
}

/// Path to application's root directory
pub trait RootPath {
    /// Get the path to the application's root directory
    fn root(&self) -> &AbsPath;
}

/// Path to the application's secrets directory
pub trait SecretsPath {
    /// Get the path to the application's secrets directory
    fn secrets(&self) -> &AbsPath;
}

/// Standard set of "happy paths" used by Abscissa applications.
///
/// These are not yet finalized, but provide a standard application layout
/// (further expressed in the template) which future Abscissa
/// components/extensions should seek to adhere to.
#[derive(Clone, Debug)]
pub struct StandardPaths {
    /// Path to the application's executable.
    exe: AbsPathBuf,

    /// Path to the application's root directory
    root: AbsPathBuf,

    /// Path to the application's secrets
    secrets: Option<AbsPathBuf>,
}

impl StandardPaths {
    /// Compute paths to application resources from the path of the
    /// application's executable:
    ///
    /// - `./` (root): application root directory
    /// - `./{{~name~}}` (bin): application executable path
    /// - `./secrets` (secrets): location of files containing app's secrets
    fn from_exe_path<P>(exe_path: P) -> Result<Self, FrameworkError>
    where
        P: Into<AbsPathBuf>,
    {
        let exe = exe_path.into();
        let root = exe.parent()?;
        let secrets = root.join(SECRETS_DIR).ok();
        Ok(StandardPaths { exe, root, secrets })
    }
}

impl Default for StandardPaths {
    // TODO(tarcieri): better error handling for canonicalization failures
    fn default() -> Self {
        let exe_path = canonical_path::current_exe().unwrap_or_else(|e| {
            panic!("error canonicalizing application path: {}", e);
        });

        Self::from_exe_path(exe_path).unwrap_or_else(|e| {
            panic!("error computing application paths: {}", e);
        })
    }
}

impl ExePath for StandardPaths {
    fn exe(&self) -> &AbsPath {
        self.exe.as_ref()
    }
}

impl RootPath for StandardPaths {
    fn root(&self) -> &AbsPath {
        self.root.as_ref()
    }
}

impl SecretsPath for StandardPaths {
    fn secrets(&self) -> &AbsPath {
        self.secrets
            .as_ref()
            .unwrap_or_else(|| {
                // TODO(tarcieri): better error handling for this case
                panic!("secrets directory does not exist");
            })
            .as_ref()
    }
}
