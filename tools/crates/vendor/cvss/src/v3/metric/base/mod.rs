//! CVSS v3.1 Base Metric Group

mod a;
mod ac;
mod av;
mod c;
mod i;
mod pr;
mod s;
mod ui;

pub use self::{
    a::Availability, ac::AttackComplexity, av::AttackVector, c::Confidentiality, i::Integrity,
    pr::PrivilegesRequired, s::Scope, ui::UserInteraction,
};
