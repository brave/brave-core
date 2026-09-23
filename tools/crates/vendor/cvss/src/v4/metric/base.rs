//! CVSS v4.0 Base Metric Group

mod ac;
mod at;
mod av;
mod pr;
mod sa;
mod sc;
mod si;
mod ui;
mod va;
mod vc;
mod vi;

pub use self::{
    ac::AttackComplexity, at::AttackRequirements, av::AttackVector, pr::PrivilegesRequired,
    sa::AvailabilityImpactToTheSubsequentSystem, sc::ConfidentialityImpactToTheSubsequentSystem,
    si::IntegrityImpactToTheSubsequentSystem, ui::UserInteraction,
    va::AvailabilityImpactToTheVulnerableSystem, vc::ConfidentialityImpactToTheVulnerableSystem,
    vi::IntegrityImpactToTheVulnerableSystem,
};
#[cfg(feature = "std")]
pub(crate) use self::{
    ac::merge::MergedAttackComplexity, at::merge::MergedAttackRequirements,
    av::merge::MergedAttackVector, pr::merge::MergedPrivilegesRequired,
    sa::merge::MergedAvailabilityImpactToTheSubsequentSystem,
    sc::merge::MergedConfidentialityImpactToTheSubsequentSystem,
    si::merge::MergedIntegrityImpactToTheSubsequentSystem, ui::merge::MergedUserInteraction,
    va::merge::MergedAvailabilityImpactToTheVulnerableSystem,
    vc::merge::MergedConfidentialityImpactToTheVulnerableSystem,
    vi::merge::MergedIntegrityImpactToTheVulnerableSystem,
};
