//! CVSS v4.0 Environmental Metric Group

mod ar;
mod cr;
mod ir;
mod mac;
mod mat;
mod mav;
mod mpr;
mod msa;
mod msc;
mod msi;
mod mui;
mod mva;
mod mvc;
mod mvi;

pub use self::{
    ar::AvailabilityRequirements, cr::ConfidentialityRequirements, ir::IntegrityRequirements,
    mac::ModifiedAttackComplexity, mat::ModifiedAttackRequirements, mav::ModifiedAttackVector,
    mpr::ModifiedPrivilegesRequired, msa::ModifiedAvailabilityImpactToTheSubsequentSystem,
    msc::ModifiedConfidentialityImpactToTheSubsequentSystem,
    msi::ModifiedIntegrityImpactToTheSubsequentSystem, mui::ModifiedUserInteraction,
    mva::ModifiedAvailabilityImpactToTheVulnerableSystem,
    mvc::ModifiedConfidentialityImpactToTheVulnerableSystem,
    mvi::ModifiedIntegrityImpactToTheVulnerableSystem,
};
#[cfg(feature = "std")]
pub(crate) use self::{
    ar::merge::MergedAvailabilityRequirements, cr::merge::MergedConfidentialityRequirements,
    ir::merge::MergedIntegrityRequirements,
};
