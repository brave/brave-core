//! Core auditing functionality

use crate::config::DatabaseConfig;
use crate::{
    config::AuditConfig, error::display_err_with_source, prelude::*, presenter::Presenter,
};
use rustsec::registry::CachedIndex;
use rustsec::{Error, ErrorKind, Lockfile, Warning, WarningKind, report};

use rustsec::binary_scanning::BinaryFormat;

use std::{
    io::{self, Read},
    path::Path,
    process::exit,
    time::Duration,
};

// TODO: make configurable
const DEFAULT_LOCK_TIMEOUT: Duration = Duration::from_secs(5 * 60);

#[cfg(feature = "binary-scanning")]
const DEFAULT_MAX_BINARY_SIZE: u64 = 100 * 1024 * 1024; // 100MB

/// Security vulnerability auditor
pub struct Auditor {
    /// RustSec Advisory Database
    database: rustsec::Database,

    /// Crates.io registry index
    registry_index: Option<CachedIndex>,

    /// Presenter for displaying the report
    presenter: Presenter,

    /// Audit report settings
    report_settings: report::Settings,

    /// Binary scanning configuration (max input size)
    #[cfg(feature = "binary-scanning")]
    binary_size_limit: Option<u64>,

    /// Binary scanning configuration (max auditable payload size)
    #[cfg(feature = "binary-scanning")]
    audit_data_size_limit: Option<usize>,
}

impl Auditor {
    /// Initialize the auditor
    pub fn new(config: &AuditConfig) -> Self {
        let advisory_db_path = config.database.path();
        let database = if config.database.fetch {
            update_db(
                &advisory_db_path,
                &config.database,
                config.output.is_quiet(),
            )
        } else {
            rustsec::Database::open(&advisory_db_path).unwrap_or_else(|e| {
                status_err!(
                    "error loading advisory database: {}",
                    display_err_with_source(&e)
                );
                exit(1);
            })
        };

        if !config.output.is_quiet() {
            status_ok!(
                "Loaded",
                "{} security advisories (from {})",
                database.iter().count(),
                advisory_db_path.display()
            );
        }

        let registry_index = if config.yanked.enabled {
            if config.yanked.update_index && config.database.fetch {
                if !config.output.is_quiet() {
                    status_ok!("Updating", "crates.io index");
                }

                let user_agent = format!("cargo-audit/{}", crate::VERSION);
                let mut result = CachedIndex::fetch_as(Duration::from_secs(0), user_agent.clone());

                // If the directory is locked, print a message and wait for it to become unlocked.
                // If we don't print the message, `cargo audit` would just hang with no explanation.
                if let Err(e) = &result
                    && e.kind() == ErrorKind::LockTimeout
                {
                    status_warn!(
                        "directory {} is locked, waiting for up to {} seconds for it to become available",
                        advisory_db_path.display(),
                        DEFAULT_LOCK_TIMEOUT.as_secs()
                    );
                    result = CachedIndex::fetch_as(DEFAULT_LOCK_TIMEOUT, user_agent);
                }

                match result {
                    Ok(index) => Some(index),
                    Err(err) => {
                        if !config.output.is_quiet() {
                            status_warn!("couldn't update crates.io index: {}", err);
                        }

                        None
                    }
                }
            } else {
                let mut result = CachedIndex::open(Duration::from_secs(0));

                // If the directory is locked, print a message and wait for it to become unlocked.
                // If we don't print the message, `cargo audit` would just hang with no explanation.
                if let Err(e) = &result
                    && e.kind() == ErrorKind::LockTimeout
                {
                    status_warn!(
                        "directory {} is locked, waiting for up to {} seconds for it to become available",
                        advisory_db_path.display(),
                        DEFAULT_LOCK_TIMEOUT.as_secs()
                    );
                    result = CachedIndex::open(DEFAULT_LOCK_TIMEOUT)
                }

                match result {
                    Ok(index) => Some(index),
                    Err(err) => {
                        if !config.output.is_quiet() {
                            status_warn!("couldn't open crates.io index: {}", err);
                        }

                        None
                    }
                }
            }
        } else {
            None
        };

        Self {
            database,
            registry_index,
            presenter: Presenter::new(&config.output),
            report_settings: config.report_settings(),
            #[cfg(feature = "binary-scanning")]
            binary_size_limit: Some(DEFAULT_MAX_BINARY_SIZE),
            #[cfg(feature = "binary-scanning")]
            audit_data_size_limit: None,
        }
    }

    /// Perform an audit of a textual `Cargo.lock` file
    pub fn audit_lockfile(&mut self, lockfile_path: &Path) -> rustsec::Result<rustsec::Report> {
        let lockfile = match self.load_lockfile(lockfile_path) {
            Ok(l) => l,
            Err(e) => {
                return Err(Error::with_source(
                    ErrorKind::NotFound,
                    format!("Couldn't load {}", lockfile_path.display()),
                    e,
                ));
            }
        };

        self.presenter.before_report(lockfile_path, &lockfile);

        let report = self.audit(&lockfile, None, None);

        let self_advisories = self.self_advisories();

        self.presenter.print_self_report(self_advisories.as_slice());

        report
    }

    #[cfg(feature = "binary-scanning")]
    /// Perform an audit of multiple binary files
    pub fn audit_binaries<P>(&mut self, binaries: &[P]) -> MultiFileReportSummmary
    where
        P: AsRef<Path>,
    {
        let mut summary = MultiFileReportSummmary::default();
        for path in binaries {
            let result = self.audit_binary(path.as_ref());
            match result {
                Ok(report) => {
                    if self.presenter.should_exit_with_failure(&report) {
                        summary.vulnerabilities_found = true;
                    }
                }
                Err(e) => {
                    status_err!("{}", display_err_with_source(&e));
                    summary.errors_encountered = true;
                }
            }
        }

        let self_advisories = self.self_advisories();

        self.presenter.print_self_report(self_advisories.as_slice());

        if self
            .presenter
            .should_exit_with_failure_due_to_self(&self.self_advisories())
        {
            summary.errors_encountered = true;
        }
        summary
    }

    #[cfg(feature = "binary-scanning")]
    /// Configure binary-scanning limits for this `Auditor`.
    ///
    /// `max_binary_size` is in bytes. If unset, defaults to 100MB.
    ///
    /// `audit_data_size_limit` is the maximum size (in bytes) of embedded auditable payload data
    /// to parse. If unset, the default from `rustsec` applies (currently 8MB).
    pub fn set_binary_scan_limits(
        &mut self,
        max_binary_size: Option<u64>,
        audit_data_size_limit: Option<usize>,
    ) {
        self.binary_size_limit = Some(max_binary_size.unwrap_or(DEFAULT_MAX_BINARY_SIZE));
        self.audit_data_size_limit = audit_data_size_limit.or(Some(8 * 1024 * 1024));
    }

    #[cfg(feature = "binary-scanning")]
    /// Perform an audit of a binary file with dependency data embedded by `cargo auditable`
    fn audit_binary(&mut self, binary_path: &Path) -> rustsec::Result<rustsec::Report> {
        use rustsec::binary_scanning::BinaryReport::*;
        let file_contents = self.read_binary_with_limit(binary_path)?;
        let (binary_type, report) = rustsec::binary_scanning::load_deps_from_binary(
            &file_contents,
            self.audit_data_size_limit,
        )?;
        self.presenter.binary_scan_report(&report, binary_path);
        self.presenter.set_binary_contents(file_contents);
        match report {
            Complete(lockfile) | Incomplete(lockfile) => {
                self.audit(&lockfile, Some(binary_path), Some(binary_type))
            }
            None => Err(Error::new(
                ErrorKind::Parse,
                "No dependency information found! Is this a Rust executable built with cargo?",
            )),
        }
    }

    #[cfg(feature = "binary-scanning")]
    fn read_binary_with_limit(&self, binary_path: &Path) -> rustsec::Result<Vec<u8>> {
        let file = std::fs::File::open(binary_path)?;
        let limit = self.binary_size_limit.unwrap_or(DEFAULT_MAX_BINARY_SIZE);
        let mut limited = file.take(limit.saturating_add(1));
        let mut buffer = Vec::new();
        limited.read_to_end(&mut buffer)?;
        if buffer.len() as u64 > limit {
            return Err(Error::new(
                ErrorKind::BadParam,
                format!(
                    "binary {} exceeds max size limit of {} bytes",
                    binary_path.display(),
                    limit
                ),
            ));
        }
        Ok(buffer)
    }

    /// The part of the auditing process that is shared between auditing lockfiles and binary files
    fn audit(
        &mut self,
        lockfile: &Lockfile,
        path: Option<&Path>,
        #[allow(unused_variables)] // May be unused when the "binary-scanning" feature is disabled
        binary_format: Option<BinaryFormat>,
    ) -> rustsec::Result<rustsec::Report> {
        let mut report = rustsec::Report::generate(&self.database, lockfile, &self.report_settings);

        #[cfg(feature = "binary-scanning")]
        if let Some(format) = binary_format {
            use rustsec::binary_scanning::filter_report_by_binary_type;
            filter_report_by_binary_type(&format, &mut report);
        }

        // Warn for yanked crates
        let mut yanked = self.check_for_yanked_crates(lockfile);
        if !yanked.is_empty() {
            report
                .warnings
                .entry(WarningKind::Yanked)
                .or_default()
                .append(&mut yanked);
        }

        self.presenter.mark_denied_warnings(&mut report);
        self.presenter.print_report(&report, lockfile, path);

        Ok(report)
    }

    fn check_for_yanked_crates(&mut self, lockfile: &Lockfile) -> Vec<Warning> {
        let mut result = Vec::new();
        if let Some(index) = &mut self.registry_index {
            let pkgs_to_check: Vec<_> = lockfile
                .packages
                .iter()
                .filter(|pkg| match &pkg.source {
                    Some(source) => source.is_default_registry(),
                    None => false,
                })
                .collect();

            let yanked = index.find_yanked(pkgs_to_check);

            for pkg in yanked {
                match pkg {
                    Ok(pkg) => {
                        let warning = Warning::new(WarningKind::Yanked, pkg, None, None, None);
                        result.push(warning);
                    }
                    Err(e) => {
                        status_err!(
                            "couldn't check if the package is yanked: {}",
                            display_err_with_source(&e)
                        );
                    }
                }
            }
        }
        result
    }

    /// Load the lockfile to be audited
    fn load_lockfile(&self, lockfile_path: &Path) -> rustsec::Result<Lockfile> {
        if lockfile_path == Path::new("-") {
            // Read Cargo.lock from STDIN
            let mut lockfile_toml = String::new();
            io::stdin().read_to_string(&mut lockfile_toml)?;
            Ok(lockfile_toml.parse()?)
        } else {
            Ok(Lockfile::load(lockfile_path)?)
        }
    }

    /// Query the database for advisories about `cargo-audit` or `rustsec` itself
    fn self_advisories(&self) -> Vec<rustsec::Advisory> {
        let mut results = vec![];

        for (package_name, package_version) in [
            ("cargo-audit", crate::VERSION),
            ("rustsec", rustsec::VERSION),
        ] {
            let query = rustsec::database::Query::crate_scope()
                .package_name(package_name.parse().unwrap())
                .package_version(package_version.parse().unwrap());

            for advisory in self.database.query(&query) {
                results.push(advisory.clone());
            }
        }

        results
    }

    /// Determines whether the process should exit with failure based on configuration
    /// such as `--deny=warnings`.
    /// **Performance:** calls `Auditor.self_advisories()`, which is costly.
    /// Do not call this in a hot loop.
    pub fn should_exit_with_failure(&self, report: &rustsec::Report) -> bool {
        self.presenter.should_exit_with_failure(report)
            || self
                .presenter
                .should_exit_with_failure_due_to_self(&self.self_advisories())
    }
}

pub(crate) fn update_db(path: &Path, config: &DatabaseConfig, quiet: bool) -> rustsec::Database {
    let url = config.url();
    if !quiet {
        status_ok!("Fetching", "advisory database from `{}`", url);
    }

    let mut result = rustsec::repository::git::Repository::fetch(
        url,
        path,
        !config.stale,
        Duration::from_secs(0),
    );
    // If the directory is locked, print a message and wait for it to become unlocked.
    // If we don't print the message, `cargo audit` would just hang with no explanation.
    if let Err(e) = &result
        && e.kind() == ErrorKind::LockTimeout
    {
        status_warn!(
            "directory {} is locked, waiting for up to {} seconds for it to become available",
            path.display(),
            DEFAULT_LOCK_TIMEOUT.as_secs()
        );
        result = rustsec::repository::git::Repository::fetch(
            url,
            path,
            !config.stale,
            DEFAULT_LOCK_TIMEOUT,
        );
    }

    let advisory_db_repo = result.unwrap_or_else(|e| {
        status_err!(
            "couldn't fetch advisory database: {}",
            display_err_with_source(&e)
        );
        exit(1);
    });

    rustsec::Database::load_from_repo(&advisory_db_repo).unwrap_or_else(|e| {
        status_err!(
            "error loading advisory database: {}",
            display_err_with_source(&e)
        );
        exit(1);
    })
}

/// Summary of the report over multiple scanned files
#[derive(Clone, Copy, Debug, Default)]
pub struct MultiFileReportSummmary {
    /// Whether any vulnerabilities were found
    pub vulnerabilities_found: bool,
    /// Whether any errors were encountered during scanning
    pub errors_encountered: bool,
}
