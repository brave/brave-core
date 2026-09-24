use alloc::boxed::Box;
use alloc::str::FromStr;
use core::fmt;

#[cfg(feature = "v3")]
use crate::v3;
#[cfg(feature = "v4")]
use crate::v4;
use crate::{
    Severity,
    error::{Error, Result},
};
use alloc::borrow::ToOwned;
#[cfg(feature = "serde")]
use {
    alloc::string::String,
    alloc::string::ToString,
    serde::{Deserialize, Serialize, de, ser},
};

/// Prefix used by all CVSS strings
pub const PREFIX: &str = "CVSS";

/// A CVSS vector
#[derive(Clone, PartialEq, Eq, Debug)]
#[non_exhaustive]
pub enum Cvss {
    #[cfg(feature = "v3")]
    /// A CVSS 3.0 base vector
    CvssV30(v3::Base),
    #[cfg(feature = "v3")]
    /// A CVSS 3.1 base vector
    CvssV31(v3::Base),
    #[cfg(feature = "v4")]
    /// A CVSS 4.0 vector
    CvssV40(v4::Vector),
}

impl Cvss {
    /// Get the score of this CVSS vector
    ///
    /// The different versions of CVSS have dedicated `Score` types.
    /// For CVSSv4 specifically, the dedicated type includes the nomenclature information.
    #[cfg(feature = "std")]
    pub fn score(&self) -> f64 {
        match self {
            #[cfg(feature = "v3")]
            Self::CvssV30(base) => base.score().value(),
            #[cfg(feature = "v3")]
            Self::CvssV31(base) => base.score().value(),
            #[cfg(feature = "v4")]
            Self::CvssV40(vector) => vector.score().value(),
        }
    }

    /// Get the severity of this CVSS vector
    #[cfg(feature = "std")]
    pub fn severity(&self) -> Severity {
        match self {
            #[cfg(feature = "v3")]
            Self::CvssV30(base) => base.score().severity(),
            #[cfg(feature = "v3")]
            Self::CvssV31(base) => base.score().severity(),
            #[cfg(feature = "v4")]
            Self::CvssV40(vector) => vector.score().severity(),
        }
    }

    /// Get an iterator over all defined metrics
    pub fn metrics(&self) -> Box<dyn Iterator<Item = (MetricType, &dyn fmt::Debug)> + '_> {
        match self {
            #[cfg(feature = "v3")]
            Self::CvssV30(base) => Box::new(base.metrics().map(|(m, v)| (MetricType::V3(m), v))),
            #[cfg(feature = "v3")]
            Self::CvssV31(base) => Box::new(base.metrics().map(|(m, v)| (MetricType::V3(m), v))),
            #[cfg(feature = "v4")]
            Self::CvssV40(vector) => {
                Box::new(vector.metrics().map(|(m, v)| (MetricType::V4(m), v)))
            }
        }
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl<'de> Deserialize<'de> for Cvss {
    fn deserialize<D: de::Deserializer<'de>>(
        deserializer: D,
    ) -> core::result::Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

#[cfg(feature = "serde")]
#[cfg_attr(docsrs, doc(cfg(feature = "serde")))]
impl Serialize for Cvss {
    fn serialize<S: ser::Serializer>(
        &self,
        serializer: S,
    ) -> core::result::Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}

impl FromStr for Cvss {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        // Parse the prefix and select the right vector parser
        let (id, _) = s.split_once('/').ok_or(Error::InvalidComponent {
            component: s.to_owned(),
        })?;
        let (prefix, version) = id.split_once(':').ok_or(Error::InvalidComponent {
            component: id.to_owned(),
        })?;
        let (major_version, minor_version) =
            version.split_once('.').ok_or(Error::InvalidComponent {
                component: id.to_owned(),
            })?;

        match (prefix, major_version, minor_version) {
            #[cfg(feature = "v3")]
            (PREFIX, "3", "0") => v3::Base::from_str(s).map(Self::CvssV30),
            #[cfg(feature = "v3")]
            (PREFIX, "3", "1") => v3::Base::from_str(s).map(Self::CvssV31),
            #[cfg(feature = "v4")]
            (PREFIX, "4", "0") => v4::Vector::from_str(s).map(Self::CvssV40),
            (PREFIX, _, _) => Err(Error::UnsupportedVersion {
                version: version.to_owned(),
            }),
            (_, _, _) => Err(Error::InvalidPrefix {
                prefix: prefix.to_owned(),
            }),
        }
    }
}

impl fmt::Display for Cvss {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            #[cfg(feature = "v3")]
            Self::CvssV30(base) => write!(f, "{}", base),
            #[cfg(feature = "v3")]
            Self::CvssV31(base) => write!(f, "{}", base),
            #[cfg(feature = "v4")]
            Self::CvssV40(vector) => write!(f, "{}", vector),
        }
    }
}

/// Metric type (across CVSS versions)
#[non_exhaustive]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum MetricType {
    V3(v3::MetricType),
    V4(v4::MetricType),
}

impl MetricType {
    /// Get the name of this metric (i.e. acronym)
    pub fn name(self) -> &'static str {
        match self {
            Self::V3(m) => m.name(),
            Self::V4(m) => m.name(),
        }
    }

    /// Get a description of this metric.
    pub fn description(self) -> &'static str {
        match self {
            Self::V3(m) => m.description(),
            Self::V4(m) => m.description(),
        }
    }
}

#[cfg(all(feature = "std", test))]
mod tests {
    use super::{Cvss, Error};
    use alloc::borrow::ToOwned;

    #[test]
    #[cfg(feature = "v3")]
    fn test_parse_v3() {
        let vector = "CVSS:3.0/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N".parse::<Cvss>();
        assert!(vector.is_ok());

        let vector = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N".parse::<Cvss>();
        assert!(vector.is_ok());
    }

    #[test]
    #[cfg(feature = "v4")]
    fn test_parse_v4() {
        let vector = "CVSS:4.0/AV:P/AC:H/AT:P/PR:L/UI:P/VC:H/VI:H/VA:H/SC:L/SI:L/SA:L/E:A/S:P/AU:Y/R:A/V:D/RE:L/U:Red".parse::<Cvss>();
        assert!(vector.is_ok());
    }

    #[test]
    fn test_parse_invalid() {
        let err = "CVSS:5.0/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N"
            .parse::<Cvss>()
            .unwrap_err();
        assert_eq!(
            err,
            Error::UnsupportedVersion {
                version: "5.0".to_owned()
            }
        );
        let err = "CSS:4.0/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N"
            .parse::<Cvss>()
            .unwrap_err();
        assert_eq!(
            err,
            Error::InvalidPrefix {
                prefix: "CSS".to_owned()
            }
        );
        let err = "garbage/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N"
            .parse::<Cvss>()
            .unwrap_err();
        assert_eq!(
            err,
            Error::InvalidComponent {
                component: "garbage".to_owned()
            }
        );
    }
}
