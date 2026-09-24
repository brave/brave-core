#![doc = include_str!("../README.md")]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/iqlusioninc/abscissa/main/img/abscissa-sq.svg"
)]
#![forbid(unsafe_code)]
#![warn(rust_2018_idioms, unused_lifetimes, unused_qualifications)]

mod command;
mod component;
mod runnable;

use synstructure::decl_derive;

decl_derive!([Command] => command::derive_command);
decl_derive!([Component, attributes(component)] => component::derive_component);
decl_derive!([Runnable] => runnable::derive_runnable);
