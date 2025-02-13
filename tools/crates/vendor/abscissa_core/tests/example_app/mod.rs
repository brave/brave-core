//! Example application used for testing purposes

use abscissa_core::{
    application, clap::Parser, config, Application, Command, Configurable, FrameworkError,
    Runnable, StandardPaths,
};
use serde::{Deserialize, Serialize};
use std::path::PathBuf;

#[derive(Clone, Debug, Default, Deserialize, Serialize)]
pub struct ExampleConfig {}

#[derive(Command, Debug, Parser)]
pub struct ExampleCommand {}

impl Configurable<ExampleConfig> for ExampleCommand {
    fn config_path(&self) -> Option<PathBuf> {
        None
    }
}

impl Runnable for ExampleCommand {
    fn run(&self) {
        unimplemented!();
    }
}

#[derive(Debug, Default)]
pub struct ExampleApp {
    config: Option<ExampleConfig>,
    state: application::State<Self>,
}

impl Application for ExampleApp {
    type Cmd = ExampleCommand;
    type Cfg = ExampleConfig;
    type Paths = StandardPaths;

    fn config(&self) -> config::Reader<ExampleConfig> {
        unimplemented!();
    }

    fn state(&self) -> &application::State<Self> {
        unimplemented!();
    }

    fn register_components(&mut self, command: &Self::Cmd) -> Result<(), FrameworkError> {
        let framework_components = self.framework_components(command)?;
        let mut app_components = self.state.components_mut();
        app_components.register(framework_components)
    }

    fn after_config(&mut self, config: Self::Cfg) -> Result<(), FrameworkError> {
        let mut components = self.state.components_mut();
        components.after_config(&config)?;
        self.config = Some(config);
        Ok(())
    }
}
