use std::str::FromStr;

use clap::Parser;
use raw_cpuid::CpuId;

enum OutputFormat {
    Raw,
    Json,
    Cli,
}

impl FromStr for OutputFormat {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "raw" => Ok(OutputFormat::Raw),
            "json" => Ok(OutputFormat::Json),
            "cli" => Ok(OutputFormat::Cli),
            _ => Err("no match"),
        }
    }
}

/// Prints information about the current x86 CPU to stdout using the cpuid instruction.
#[derive(Parser)]
#[clap(version = "10.2", author = "Gerd Zellweger <mail@gerdzellweger.com>")]
#[clap(disable_colored_help(true))]
struct Opts {
    /// Configures the output format.
    #[clap(short, long, default_value = "cli", possible_values = &["raw", "json", "cli", ])]
    format: OutputFormat,
}

fn main() {
    let opts: Opts = Opts::parse();
    match opts.format {
        OutputFormat::Raw => raw_cpuid::display::raw(),
        OutputFormat::Json => {
            let cpuid = CpuId::new();
            raw_cpuid::display::json(cpuid);
        }
        OutputFormat::Cli => {
            let cpuid = CpuId::new();
            raw_cpuid::display::markdown(cpuid);
        }
    };
}
