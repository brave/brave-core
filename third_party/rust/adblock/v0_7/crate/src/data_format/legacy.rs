//! Contains representations of data from the adblocking engine in a
//! forwards-and-backwards-compatible format, as well as utilities for converting these to and from
//! the actual `Engine` components.
//!
//! The format itself is split into two parts for historical reasons. Any new fields should be
//! added to the _end_ of both `SerializeFormatRest` and `DeserializeFormatRest`.
//!
//! This particular data format is space-inefficient, has several unused fields, prevents some
//! dependency updates, and the lack of a version field makes upgrades difficult. It will be
//! removed in a future release.

use std::collections::{HashMap, HashSet};

use flate2::read::GzDecoder;
use flate2::write::GzEncoder;
use flate2::Compression;
use rmp_serde_legacy as rmps;
use serde::{Deserialize, Serialize};

use crate::blocker::{Blocker, NetworkFilterList};
use crate::cosmetic_filter_cache::{CosmeticFilterCache, HostnameRuleDb};
use crate::filters::network::NetworkFilter;
use crate::resources::{RedirectResourceStorage, ScriptletResourceStorage};
use crate::utils::is_eof_error;

use super::{DeserializationError, SerializationError};

/// `_fuzzy_signature` is no longer used, and is removed from future format versions.
#[derive(Debug, Clone, Serialize)]
struct NetworkFilterLegacySerializeFmt<'a> {
    mask: &'a crate::filters::network::NetworkFilterMask,
    filter: &'a crate::filters::network::FilterPart,
    opt_domains: &'a Option<Vec<crate::utils::Hash>>,
    opt_not_domains: &'a Option<Vec<crate::utils::Hash>>,
    redirect: &'a Option<String>,
    hostname: &'a Option<String>,
    csp: &'a Option<String>,
    bug: Option<u32>,
    tag: &'a Option<String>,
    raw_line: Option<String>,
    id: &'a crate::utils::Hash,
    _fuzzy_signature: Option<Vec<crate::utils::Hash>>,
    opt_domains_union: &'a Option<crate::utils::Hash>,
    opt_not_domains_union: &'a Option<crate::utils::Hash>,
}

/// Generic over `Borrow<NetworkFilter>` because `tagged_filters_all` requires `&'a NetworkFilter`
/// while `NetworkFilterList` requires `&'a Arc<NetworkFilter>`.
impl<'a, T> From<&'a T> for NetworkFilterLegacySerializeFmt<'a>
where
    T: std::borrow::Borrow<NetworkFilter>,
{
    fn from(v: &'a T) -> NetworkFilterLegacySerializeFmt<'a> {
        let v = v.borrow();
        NetworkFilterLegacySerializeFmt {
            mask: &v.mask,
            filter: &v.filter,
            opt_domains: &v.opt_domains,
            opt_not_domains: &v.opt_not_domains,
            redirect: if v.is_redirect() {
                &v.modifier_option
            } else {
                &None
            },
            hostname: &v.hostname,
            csp: if v.is_csp() {
                &v.modifier_option
            } else {
                &None
            },
            bug: None,
            tag: &v.tag,
            raw_line: v.raw_line.as_ref().map(|raw| *raw.clone()),
            id: &v.id,
            _fuzzy_signature: None,
            opt_domains_union: &v.opt_domains_union,
            opt_not_domains_union: &v.opt_not_domains_union,
        }
    }
}

/// Forces a `NetworkFilterList` to be serialized with the legacy filter format by converting to an
/// intermediate representation that is constructed with `NetworkFilterLegacyFmt` instead.
fn serialize_legacy_network_filter_list<S>(
    list: &NetworkFilterList,
    s: S,
) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
{
    #[derive(Serialize, Default)]
    struct NetworkFilterListLegacySerializeFmt<'a> {
        #[serde(serialize_with = "crate::data_format::utils::stabilize_hashmap_serialization")]
        filter_map: HashMap<crate::utils::Hash, Vec<NetworkFilterLegacySerializeFmt<'a>>>,
    }

    let legacy_list = NetworkFilterListLegacySerializeFmt {
        filter_map: list
            .filter_map
            .iter()
            .map(|(k, v)| (*k, v.iter().map(|f| f.into()).collect()))
            .collect(),
    };

    legacy_list.serialize(s)
}

/// Forces a `NetworkFilter` slice to be serialized with the legacy filter format by converting to
/// an intermediate representation that is constructed with `NetworkFilterLegacyFmt` instead.
fn serialize_legacy_network_filter_vec<S>(vec: &[NetworkFilter], s: S) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
{
    let legacy_vec: Vec<_> = vec
        .iter()
        .map(NetworkFilterLegacySerializeFmt::from)
        .collect();

    legacy_vec.serialize(s)
}

/// Provides structural aggregration of referenced adblock engine data to allow for allocation-free
/// serialization.
///
/// Note that this does not implement `Serialize` directly, as it is composed of two parts which
/// must be serialized independently. Instead, use the `serialize` method.
pub struct SerializeFormat<'a> {
    part1: SerializeFormatPt1<'a>,
    rest: SerializeFormatRest<'a>,
}

impl<'a> SerializeFormat<'a> {
    pub fn serialize(&self) -> Result<Vec<u8>, SerializationError> {
        let mut gz = GzEncoder::new(Vec::new(), Compression::default());
        rmps::encode::write(&mut gz, &self.part1)?;
        rmps::encode::write(&mut gz, &self.rest)?;
        let compressed = gz.finish()?;
        Ok(compressed)
    }
}

#[derive(Serialize)]
struct SerializeFormatPt1<'a> {
    #[serde(serialize_with = "serialize_legacy_network_filter_list")]
    csp: &'a NetworkFilterList,
    #[serde(serialize_with = "serialize_legacy_network_filter_list")]
    exceptions: &'a NetworkFilterList,
    #[serde(serialize_with = "serialize_legacy_network_filter_list")]
    importants: &'a NetworkFilterList,
    #[serde(serialize_with = "serialize_legacy_network_filter_list")]
    redirects: &'a NetworkFilterList,
    #[serde(serialize_with = "serialize_legacy_network_filter_list")]
    filters_tagged: &'a NetworkFilterList,
    #[serde(serialize_with = "serialize_legacy_network_filter_list")]
    filters: &'a NetworkFilterList,

    #[serde(serialize_with = "serialize_legacy_network_filter_vec")]
    tagged_filters_all: &'a Vec<NetworkFilter>,

    _debug: bool,
    enable_optimizations: bool,

    // This field exists for backwards compatibility only.
    _unused: bool,
    // This field exists for backwards compatibility only, and *must* be true.
    _unused2: bool,

    resources: &'a RedirectResourceStorage,
}

#[derive(Serialize)]
struct SerializeFormatRest<'a> {
    simple_class_rules: &'a HashSet<String>,
    simple_id_rules: &'a HashSet<String>,
    complex_class_rules: &'a HashMap<String, Vec<String>>,
    complex_id_rules: &'a HashMap<String, Vec<String>>,

    specific_rules: &'a HostnameRuleDb,

    misc_generic_selectors: &'a HashSet<String>,

    scriptlets: &'a ScriptletResourceStorage,

    #[serde(serialize_with = "serialize_legacy_network_filter_list")]
    generic_hide: &'a NetworkFilterList,
}

/// `_fuzzy_signature` is no longer used, and is cleaned up from future format versions.
#[derive(Debug, Clone, Deserialize)]
pub(crate) struct NetworkFilterLegacyDeserializeFmt {
    pub mask: crate::filters::network::NetworkFilterMask,
    pub filter: crate::filters::network::FilterPart,
    pub opt_domains: Option<Vec<crate::utils::Hash>>,
    pub opt_not_domains: Option<Vec<crate::utils::Hash>>,
    pub redirect: Option<String>,
    pub hostname: Option<String>,
    pub csp: Option<String>,
    pub bug: Option<u32>,
    pub tag: Option<String>,
    pub raw_line: Option<String>,
    pub id: crate::utils::Hash,
    _fuzzy_signature: Option<Vec<crate::utils::Hash>>,
    pub opt_domains_union: Option<crate::utils::Hash>,
    pub opt_not_domains_union: Option<crate::utils::Hash>,
}

impl From<NetworkFilterLegacyDeserializeFmt> for NetworkFilter {
    fn from(v: NetworkFilterLegacyDeserializeFmt) -> Self {
        Self {
            mask: v.mask,
            filter: v.filter,
            opt_domains: v.opt_domains,
            opt_not_domains: v.opt_not_domains,
            modifier_option: v.redirect.or(v.csp),
            hostname: v.hostname,
            tag: v.tag,
            raw_line: v.raw_line.map(Box::new),
            id: v.id,
            opt_domains_union: v.opt_domains_union,
            opt_not_domains_union: v.opt_not_domains_union,
        }
    }
}

#[derive(Debug, Deserialize, Default)]
pub(crate) struct NetworkFilterListLegacyDeserializeFmt {
    pub filter_map: HashMap<crate::utils::Hash, Vec<NetworkFilterLegacyDeserializeFmt>>,
}

impl From<NetworkFilterListLegacyDeserializeFmt> for NetworkFilterList {
    fn from(v: NetworkFilterListLegacyDeserializeFmt) -> Self {
        Self {
            filter_map: v
                .filter_map
                .into_iter()
                .map(|(k, v)| {
                    (
                        k,
                        v.into_iter()
                            .map(|f| std::sync::Arc::new(f.into()))
                            .collect(),
                    )
                })
                .collect(),
        }
    }
}

/// Structural representation of adblock engine data that can be built up from deserialization and
/// used directly to construct new `Engine` components without unnecessary allocation.
///
/// Note that this does not implement `Deserialize` directly, as it is composed of two parts which
/// must be deserialized independently. Instead, use the `deserialize` method.
pub struct DeserializeFormat {
    part1: DeserializeFormatPart1,
    rest: DeserializeFormatRest,
}

impl DeserializeFormat {
    pub fn deserialize(serialized: &[u8]) -> Result<Self, DeserializationError> {
        let mut gz = GzDecoder::new(serialized);
        let part1: DeserializeFormatPart1 = rmps::decode::from_read(&mut gz)?;
        let rest = match rmps::decode::from_read(&mut gz) {
            Ok(rest) => rest,
            Err(ref e) if is_eof_error(e) => Default::default(),
            Err(e) => return Err(e.into()),
        };
        Ok(Self { part1, rest })
    }
}

#[derive(Deserialize)]
struct DeserializeFormatPart1 {
    csp: NetworkFilterListLegacyDeserializeFmt,
    exceptions: NetworkFilterListLegacyDeserializeFmt,
    importants: NetworkFilterListLegacyDeserializeFmt,
    redirects: NetworkFilterListLegacyDeserializeFmt,
    filters_tagged: NetworkFilterListLegacyDeserializeFmt,
    filters: NetworkFilterListLegacyDeserializeFmt,

    tagged_filters_all: Vec<NetworkFilterLegacyDeserializeFmt>,

    debug: bool,
    enable_optimizations: bool,

    // This field exists for backwards compatibility only.
    _unused: bool,
    // This field exists for backwards compatibility only, and *must* be true.
    _unused2: bool,

    #[serde(default)]
    resources: RedirectResourceStorage,
}

/// Any fields added to this must include the `#[serde(default)]` annotation, or another serde
/// annotation that will allow the format to gracefully handle missing fields when deserializing
/// from older versions of the format.
#[derive(Deserialize, Default)]
struct DeserializeFormatRest {
    #[serde(default)]
    simple_class_rules: HashSet<String>,
    #[serde(default)]
    simple_id_rules: HashSet<String>,
    #[serde(default)]
    complex_class_rules: HashMap<String, Vec<String>>,
    #[serde(default)]
    complex_id_rules: HashMap<String, Vec<String>>,

    #[serde(default)]
    specific_rules: HostnameRuleDb,

    #[serde(default)]
    misc_generic_selectors: HashSet<String>,

    #[serde(default)]
    scriptlets: ScriptletResourceStorage,

    #[serde(default)]
    generic_hide: NetworkFilterListLegacyDeserializeFmt,
}

impl<'a> From<(&'a Blocker, &'a CosmeticFilterCache)> for SerializeFormat<'a> {
    fn from(v: (&'a Blocker, &'a CosmeticFilterCache)) -> Self {
        let (blocker, cfc) = v;
        Self {
            part1: SerializeFormatPt1 {
                csp: &blocker.csp,
                exceptions: &blocker.exceptions,
                importants: &blocker.importants,
                redirects: &blocker.redirects,
                filters_tagged: &blocker.filters_tagged,
                filters: &blocker.filters,

                tagged_filters_all: &blocker.tagged_filters_all,

                _debug: true,
                enable_optimizations: blocker.enable_optimizations,
                _unused: true,
                _unused2: true,

                resources: &blocker.resources,
            },
            rest: SerializeFormatRest {
                simple_class_rules: &cfc.simple_class_rules,
                simple_id_rules: &cfc.simple_id_rules,
                complex_class_rules: &cfc.complex_class_rules,
                complex_id_rules: &cfc.complex_id_rules,

                specific_rules: &cfc.specific_rules,

                misc_generic_selectors: &cfc.misc_generic_selectors,

                scriptlets: &cfc.scriptlets,

                generic_hide: &blocker.generic_hide,
            },
        }
    }
}

impl From<DeserializeFormat> for (Blocker, CosmeticFilterCache) {
    fn from(v: DeserializeFormat) -> Self {
        (
            Blocker {
                csp: v.part1.csp.into(),
                exceptions: v.part1.exceptions.into(),
                importants: v.part1.importants.into(),
                redirects: v.part1.redirects.into(),
                removeparam: NetworkFilterList::default(),
                filters_tagged: v.part1.filters_tagged.into(),
                filters: v.part1.filters.into(),

                tags_enabled: Default::default(),
                tagged_filters_all: v
                    .part1
                    .tagged_filters_all
                    .into_iter()
                    .map(|f| f.into())
                    .collect(),

                enable_optimizations: v.part1.enable_optimizations,

                resources: v.part1.resources,
                #[cfg(feature = "object-pooling")]
                pool: Default::default(),
                regex_manager: Default::default(),

                generic_hide: v.rest.generic_hide.into(),
            },
            CosmeticFilterCache {
                simple_class_rules: v.rest.simple_class_rules,
                simple_id_rules: v.rest.simple_id_rules,
                complex_class_rules: v.rest.complex_class_rules,
                complex_id_rules: v.rest.complex_id_rules,

                specific_rules: v.rest.specific_rules,

                misc_generic_selectors: v.rest.misc_generic_selectors,

                scriptlets: v.rest.scriptlets,
            },
        )
    }
}
