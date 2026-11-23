//! Application entrypoint for `cargo audit`

use std::{env, fs, process::exit};

use clap::Parser;

use crate::{
    commands::CargoAuditCommand,
    config::AuditConfig,
    error::{Error, ErrorKind},
    prelude::*,
    terminal::{self, ColorChoice},
};

/// Boot the application, parsing subcommand and options from command-line
/// arguments, and terminating when complete.
pub fn boot() -> ! {
    // Parse command line options
    let command = CargoAuditCommand::parse();

    // Initialize the terminal and tracing subsystems
    let color_choice = command.term_colors();
    terminal::init(color_choice);
    init_tracing(&command, color_choice);

    // Load configuration
    let config = load_config(&command).unwrap_or_else(|e| {
        status_err!("cargo-audit fatal error: {}", e);
        exit(1)
    });

    // Run the command
    command.run(&config);
    exit(0)
}

/// Load `audit.toml` (if it exists), overriding it with command-line arguments
fn load_config(command: &CargoAuditCommand) -> Result<AuditConfig, Error> {
    let config = match command.config_path() {
        Some(path) => {
            let toml_string = fs::read_to_string(&path).map_err(|e| {
                ErrorKind::Config.context(format!("couldn't read {}: {e}", path.display()))
            })?;
            toml::from_str(&toml_string).map_err(|e| ErrorKind::Parse.context(e))?
        }
        None => AuditConfig::default(),
    };

    command.process_config(config)
}

/// Initialize the `tracing` subsystem from command-line options
fn init_tracing(command: &CargoAuditCommand, color_choice: ColorChoice) {
    let filter = match command.verbose {
        true => "debug".to_owned(),
        false => env::var("RUST_LOG").unwrap_or_else(|_| "info".to_owned()),
    };

    tracing_subscriber::fmt()
        .with_ansi(color_choice != ColorChoice::Never)
        .with_env_filter(filter)
        .try_init()
        .expect("tracing subsystem failed to initialize");
}
