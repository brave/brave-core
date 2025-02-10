//! The `cargo lock` subcommand

#![forbid(unsafe_code)]
#![warn(rust_2018_idioms, unused_qualifications)]

use cargo_lock::{
    dependency::graph::EdgeDirection,
    dependency::Tree,
    package::{self},
    Dependency, Lockfile, Package, ResolveVersion, Version,
};
use gumdrop::Options;
use petgraph::graph::NodeIndex;
use std::{
    env, fs, io,
    path::{Path, PathBuf},
    process::exit,
    str::FromStr,
};

/// `cargo lock` subcommands
#[derive(Debug, Options)]
enum Command {
    /// The `cargo lock list` subcommand
    #[options(help = "list packages in Cargo.lock")]
    List(ListCmd),

    /// The `cargo lock translate` subcommand
    #[options(help = "translate a Cargo.lock file")]
    Translate(TranslateCmd),

    /// The `cargo lock tree` subcommand
    #[options(help = "print a dependency tree for the given dependency")]
    Tree(TreeCmd),
}

/// The `cargo lock list` subcommand
#[derive(Debug, Default, Options)]
struct ListCmd {
    /// Input `Cargo.lock` file
    #[options(short = "f", help = "input Cargo.lock file")]
    file: Option<PathBuf>,

    /// Get information for a specific package
    #[options(short = "p", help = "get information for a single package")]
    package: Option<package::Name>,

    /// List dependencies as part of the output
    #[options(short = "d", help = "show dependencies for each package")]
    dependencies: bool,

    /// Show package sources in list
    #[options(short = "s", help = "show package sources in listing")]
    sources: bool,
}

impl ListCmd {
    /// Display dependency summary from `Cargo.lock`
    pub fn run(&self) {
        for package in &load_lockfile(&self.file).packages {
            if let Some(name) = &self.package {
                if &package.name != name {
                    continue;
                }
            }

            if self.sources {
                println!("- {}", Dependency::from(package));
            } else {
                println!("- {} {}", package.name, package.version);
            }

            if self.dependencies {
                for dep in &package.dependencies {
                    if self.sources {
                        println!("  - {}", dep);
                    } else {
                        println!("  - {} {}", dep.name, dep.version);
                    }
                }
            }
        }
    }
}

/// The `cargo lock translate` subcommand
#[derive(Debug, Options)]
struct TranslateCmd {
    /// Input `Cargo.lock` file
    #[options(short = "f", help = "input Cargo.lock file to translate")]
    file: Option<PathBuf>,

    /// Output `Cargo.lock` file
    #[options(short = "o", help = "output Cargo.lock file (default STDOUT)")]
    output: Option<PathBuf>,

    /// Cargo.lock format version to translate to
    #[options(short = "v", help = "Cargo.lock resolve version to output")]
    version: Option<ResolveVersion>,
}

impl TranslateCmd {
    /// Translate `Cargo.lock` to a different format version
    pub fn run(&self) {
        let output = self
            .output
            .as_ref()
            .map(AsRef::as_ref)
            .unwrap_or_else(|| Path::new("-"));

        let mut lockfile = load_lockfile(&self.file);

        lockfile.version = self.version.unwrap_or_default();
        let lockfile_toml = lockfile.to_string();

        if output == Path::new("-") {
            println!("{}", &lockfile_toml);
        } else {
            fs::write(output, lockfile_toml.as_bytes()).unwrap_or_else(|e| {
                eprintln!("*** error: {}", e);
                exit(1);
            });
        }
    }
}

/// The `cargo lock tree` subcommand
#[derive(Debug, Options)]
struct TreeCmd {
    /// Input `Cargo.lock` file
    #[options(
        short = "f",
        long = "file",
        help = "input Cargo.lock file to translate"
    )]
    file: Option<PathBuf>,

    /// Show exact package identities (checksums or specific source versions) when available
    #[options(
        short = "x",
        long = "exact",
        help = "show exact package identies (checksums or specific source versions) when available"
    )]
    exact: bool,

    // Show inverse dependencies rather than forward dependencies
    #[options(
        short = "i",
        long = "invert",
        help = "show inverse dependencies _on_ a package, rather than forward dependencies _of_ a package"
    )]
    inverse: bool,

    /// Dependencies names or hashes to draw a tree for
    #[options(free, help = "dependency names or hashes to draw trees for")]
    dependencies: Vec<String>,
}

fn package_matches_name(pkg: &Package, name: &str) -> bool {
    pkg.name.as_str() == name
}

fn package_matches_ver(pkg: &Package, ver: &str) -> bool {
    // Try interpreting ver as a semver string.
    if let Ok(v) = Version::from_str(ver) {
        return pkg.version == v;
    }
    // Try comparing ver to hashes in either the package checksum or the source
    // precise field
    if let Some(cksum) = &pkg.checksum {
        if cksum.to_string() == ver {
            return true;
        }
    }
    if let Some(src) = &pkg.source {
        if let Some(precise) = src.precise() {
            if precise == ver {
                return true;
            }
        }
    }
    false
}

fn package_matches(pkg: &Package, spec: &str) -> bool {
    if let Some((name, ver)) = spec.split_once('@') {
        package_matches_name(pkg, name) && package_matches_ver(pkg, ver)
    } else {
        package_matches_name(pkg, spec) || package_matches_ver(pkg, spec)
    }
}

impl TreeCmd {
    /// Display dependency trees from `Cargo.lock`
    pub fn run(&self) {
        let lockfile = load_lockfile(&self.file);

        let tree = lockfile.dependency_tree().unwrap_or_else(|e| {
            eprintln!("*** error: {}", e);
            exit(1);
        });

        let indices: Vec<NodeIndex> = if self.dependencies.is_empty() {
            tree.roots().to_vec()
        } else {
            self.dependencies
                .iter()
                .map(|dep| {
                    let package = lockfile
                        .packages
                        .iter()
                        .find(|pkg| package_matches(pkg, dep))
                        .unwrap_or_else(|| {
                            eprintln!("*** error: invalid dependency name: `{}`", dep);
                            exit(1);
                        });
                    tree.nodes()[&package.into()]
                })
                .collect()
        };

        self.dependency_tree(&tree, &indices);
    }

    /// Show dependency tree for the provided dependencies
    fn dependency_tree(&self, tree: &Tree, indices: &[NodeIndex]) {
        for (i, index) in indices.iter().enumerate() {
            if i > 0 {
                println!();
            }
            let direction = if self.inverse {
                EdgeDirection::Incoming
            } else {
                EdgeDirection::Outgoing
            };
            tree.render(&mut io::stdout(), *index, direction, self.exact)
                .unwrap();
        }
    }
}

/// Load a lockfile from the given path (or `Cargo.toml`)
fn load_lockfile(path: &Option<PathBuf>) -> Lockfile {
    let path = path
        .as_ref()
        .map(AsRef::as_ref)
        .unwrap_or_else(|| Path::new("Cargo.lock"));

    Lockfile::load(path).unwrap_or_else(|e| {
        eprintln!("*** error: {}", e);
        exit(1);
    })
}

fn main() {
    let mut args = env::args().collect::<Vec<_>>();

    // Remove leading arguments (bin and potential `lock`)
    if !args.is_empty() {
        args.remove(0);

        if args.first().map(AsRef::as_ref) == Some("lock") {
            args.remove(0);
        }
    }

    // If no command is specified, implicitly assume `list`
    if args.is_empty() || args[0].starts_with('-') {
        ListCmd::parse_args_default(&args)
            .unwrap_or_else(|e| {
                eprintln!("*** error: {}", e);
                eprintln!("USAGE:");
                eprintln!("{}", ListCmd::usage());
                exit(1);
            })
            .run();
        exit(0);
    }

    // ...otherwise parse and run the subcommand
    let cmd = Command::parse_args_default(&args).unwrap_or_else(|e| {
        eprintln!("*** error: {}", e);
        eprintln!("USAGE:");
        eprintln!("{}", Command::usage());
        exit(1);
    });

    match cmd {
        Command::List(list) => list.run(),
        Command::Translate(translate) => translate.run(),
        Command::Tree(tree) => tree.run(),
    }
}
