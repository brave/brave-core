//! Default exit handlers for Abscissa applications

use super::{Application, Component};
use std::{error::Error, process};

/// Print a fatal error message and exit
pub fn fatal_error(app: &impl Application, err: &dyn Error) -> ! {
    status_err!("{} fatal error: {}", app.name(), err);
    process::exit(1)
}

/// Exit because component startup ordering could not be determined.
/// This is a barebones implementation using basic std facilities
/// because it might be called before the terminal component has been
/// started, and we can't use it to log errors about itself.
pub(crate) fn bad_component_order<A>(a: &dyn Component<A>, b: &dyn Component<A>) -> !
where
    A: Application,
{
    eprintln!("*** error(abscissa): couldn't determine startup order for components:");
    eprintln!(" - {:?}", a);
    eprintln!(" - {:?}", b);
    process::exit(1)
}
