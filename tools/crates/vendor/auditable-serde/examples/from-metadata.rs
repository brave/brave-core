/// Prints the audit data for the package in the current directory.
/// Passes any command-line parameters it receives to `cargo metadata`,
/// so you can specify `--filter-platform`, `--features` and other flags.
use auditable_serde::VersionInfo;
use cargo_metadata::{Metadata, MetadataCommand};

use std::{convert::TryFrom, error::Error};

fn get_metadata() -> Result<Metadata, cargo_metadata::Error> {
    let mut metadata_command = MetadataCommand::new();
    let args: Vec<String> = std::env::args().skip(1).collect();
    metadata_command.other_options(args);
    metadata_command.exec()
}

fn do_work() -> Result<(), Box<dyn Error>> {
    let stdout = std::io::stdout();
    let stdout = stdout.lock();
    let metadata = get_metadata()?;
    let version_info = VersionInfo::try_from(&metadata)?;
    serde_json::to_writer(stdout, &version_info)?;
    Ok(())
}

fn main() {
    if let Err(error) = do_work() {
        println!("{error}");
    }
}
