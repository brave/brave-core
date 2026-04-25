//! Terminal component

use crate::Component;
use termcolor::ColorChoice;

/// Abscissa terminal subsystem component
#[derive(Component, Debug)]
#[component(core)]
pub struct Terminal {}

impl Terminal {
    /// Create a new [`Terminal`] component with the given [`ColorChoice`]
    pub fn new(color_choice: ColorChoice) -> Terminal {
        // TODO(tarcieri): handle terminal reinit (without panicking)
        super::init(color_choice);

        if color_choice != ColorChoice::Never {
            // TODO(tarcieri): avoid panicking here
            color_eyre::install().expect("couldn't install color-eyre");
        }

        Self {}
    }
}
