//! CVSS v4.0 Supplemental Metric Group
//!
//! > A new, optional metric group called the Supplemental metric group provides
//! > new metrics that describe and measure additional extrinsic attributes of a
//! > vulnerability. While the assessment of Supplemental metrics is provisioned
//! > by the provider, the usage and response plan of each metric within the
//! > Supplemental metric group is determined by the consumer. This contextual
//! > information may be employed differently in each consumer’s environment. No
//! > metric will have any impact on the final calculated CVSS score (e.g.
//! > CVSS-BTE). Organizations may then assign importance and/or effective
//! > impact of each metric, or set/combination of metrics, giving them more,
//! > less, or absolutely no effect on the final risk analysis. Metrics and
//! > values will simply convey additional extrinsic characteristics of the
//! > vulnerability itself.

mod au;
mod r;
mod re;
mod s;
mod u;
mod v;

pub use self::{
    au::Automatable, r::Recovery, re::VulnerabilityResponseEffort, s::Safety, u::ProviderUrgency,
    v::ValueDensity,
};
