/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use std::collections::HashSet;
use std::str::Utf8Error;

use adblock::engine::Engine as InnerEngine;
use adblock::lists::FilterSet;
use adblock::resources::{MimeType, Resource, ResourceType};
use adblock::url_parser::ResolvesDomain;
use cxx::{let_cxx_string, CxxString, CxxVector};

use crate::ffi::{
    resolve_domain_position, BlockerDebugInfo, BlockerResult, BoxEngineResult, FilterListMetadata,
    RegexManagerDiscardPolicy, UnitResult, VecStringResult,
};
use crate::result::InternalError;

pub struct Engine {
    engine: InnerEngine,
}

impl Default for Box<Engine> {
    fn default() -> Self {
        new_engine()
    }
}

pub fn new_engine() -> Box<Engine> {
    Box::new(Engine { engine: InnerEngine::new(true) })
}

pub fn engine_with_rules(rules: &CxxVector<u8>) -> BoxEngineResult {
    || -> Result<Box<Engine>, InternalError> {
        let mut filter_set = FilterSet::new(false);
        filter_set.add_filter_list(std::str::from_utf8(rules.as_slice())?, Default::default());
        let engine = InnerEngine::from_filter_set(filter_set, true);
        Ok(Box::new(Engine { engine }))
    }()
    .into()
}

struct DomainResolver;

impl ResolvesDomain for DomainResolver {
    fn get_host_domain(&self, host: &str) -> (usize, usize) {
        let_cxx_string!(host_cxx_string = host);
        let position = resolve_domain_position(&host_cxx_string);
        (position.start as usize, position.end as usize)
    }
}

pub fn set_domain_resolver() -> bool {
    adblock::url_parser::set_domain_resolver(Box::new(DomainResolver)).is_ok()
}

pub fn read_list_metadata(list: &CxxVector<u8>) -> FilterListMetadata {
    std::str::from_utf8(list.as_slice())
        .map(|list| adblock::lists::read_list_metadata(list).into())
        .unwrap_or_default()
}

fn convert_cxx_string_vector_to_string_collection<C>(
    value: &CxxVector<CxxString>,
) -> Result<C, Utf8Error>
where
    C: FromIterator<String>,
{
    value.iter().map(|s| s.to_str().map(|t| t.to_string())).collect()
}

impl Engine {
    pub fn enable_tag(&mut self, tag: &CxxString) {
        self.engine.enable_tags(&[tag.to_str().unwrap()])
    }

    pub fn disable_tag(&mut self, tag: &CxxString) {
        self.engine.disable_tags(&[tag.to_str().unwrap()])
    }

    pub fn tag_exists(&self, key: &CxxString) -> bool {
        self.engine.tag_exists(key.to_str().unwrap())
    }

    pub fn matches(
        &self,
        url: &CxxString,
        hostname: &CxxString,
        source_hostname: &CxxString,
        request_type: &CxxString,
        third_party_request: bool,
        previously_matched_rule: bool,
        force_check_exceptions: bool,
    ) -> BlockerResult {
        // The following strings are guaranteed to be
        // UTF-8, so unwrapping directly should be okay.
        self.engine
            .check_network_urls_with_hostnames_subset(
                url.to_str().unwrap(),
                hostname.to_str().unwrap(),
                source_hostname.to_str().unwrap(),
                request_type.to_str().unwrap(),
                Some(third_party_request),
                previously_matched_rule,
                force_check_exceptions,
            )
            .into()
    }

    pub fn get_csp_directives(
        &self,
        url: &CxxString,
        hostname: &CxxString,
        source_hostname: &CxxString,
        request_type: &CxxString,
        third_party_request: bool,
    ) -> String {
        // The following strings are also UTF-8.
        self.engine
            .get_csp_directives(
                url.to_str().unwrap(),
                hostname.to_str().unwrap(),
                source_hostname.to_str().unwrap(),
                request_type.to_str().unwrap(),
                Some(third_party_request),
            )
            .unwrap_or_default()
    }

    pub fn deserialize(&mut self, serialized: &CxxVector<u8>) -> bool {
        self.engine.deserialize(serialized.as_slice()).is_ok()
    }

    pub fn add_resource(
        &mut self,
        name: &CxxString,
        content_type: &CxxString,
        data: &CxxString,
    ) -> UnitResult {
        || -> Result<(), InternalError> {
            let resource = Resource {
                name: name.to_str()?.to_string(),
                aliases: vec![],
                kind: ResourceType::Mime(MimeType::from(content_type.to_str()?)),
                content: data.to_string(),
            };
            Ok(self.engine.add_resource(resource)?)
        }()
        .into()
    }

    pub fn use_resources(&mut self, resources_json: &CxxString) -> bool {
        resources_json
            .to_str()
            .ok()
            .and_then(|resources_json| serde_json::from_str::<Vec<Resource>>(resources_json).ok())
            .and_then(|resources| {
                self.engine.use_resources(&resources);
                Some(())
            })
            .is_some()
    }

    pub fn url_cosmetic_resources(&self, url: &CxxString) -> String {
        let resources = self.engine.url_cosmetic_resources(url.to_str().unwrap());
        serde_json::to_string(&resources).unwrap()
    }

    pub fn hidden_class_id_selectors(
        &self,
        classes: &CxxVector<CxxString>,
        ids: &CxxVector<CxxString>,
        exceptions: &CxxVector<CxxString>,
    ) -> VecStringResult {
        || -> Result<Vec<String>, InternalError> {
            let classes: Vec<String> = convert_cxx_string_vector_to_string_collection(classes)?;
            let ids: Vec<String> = convert_cxx_string_vector_to_string_collection(ids)?;
            let exceptions: HashSet<String> =
                convert_cxx_string_vector_to_string_collection(exceptions)?;
            Ok(self.engine.hidden_class_id_selectors(&classes, &ids, &exceptions))
        }()
        .into()
    }

    pub fn get_debug_info(&self) -> BlockerDebugInfo {
        self.engine.get_debug_info().blocker_debug_info.into()
    }

    pub fn discard_regex(&mut self, regex_id: u64) {
        self.engine.discard_regex(regex_id)
    }

    pub fn set_regex_discard_policy(&mut self, new_discard_policy: &RegexManagerDiscardPolicy) {
        self.engine.set_regex_discard_policy(new_discard_policy.into())
    }
}
