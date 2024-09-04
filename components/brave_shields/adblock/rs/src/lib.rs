/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

//! This crate provides a cxx-based FFI for the
//! [adblock-rust](https://github.com/brave/adblock-rust) library.

mod convert;
mod engine;
mod filter_set;
mod result;

use engine::*;
use filter_set::*;

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = adblock)]
mod ffi {
    extern "Rust" {
        type FilterSet;
        fn new_filter_set() -> Box<FilterSet>;
        fn add_filter_list(&mut self, rules: &CxxVector<u8>) -> FilterListMetadataResult;
        fn add_filter_list_with_permissions(
            &mut self,
            rules: &CxxVector<u8>,
            permission_mask: u8,
        ) -> FilterListMetadataResult;
    }
    extern "Rust" {
        type Engine;
        /// Creates a new engine with no rules.
        fn new_engine() -> Box<Engine>;
        /// Creates a new engine with rules from a given filter list.
        fn engine_with_rules(rules: &CxxVector<u8>) -> BoxEngineResult;
        /// Creates a new engine with rules from a given filter set.
        fn engine_from_filter_set(filter_set: Box<FilterSet>) -> BoxEngineResult;

        /// Configures the adblock domain resolver to the call
        /// `resolve_domain_position` implemented in adblock_domain_resolver.cc.
        fn set_domain_resolver() -> bool;

        /// Extracts the homepage and title from the metadata contained in a
        /// filter list.
        fn read_list_metadata(list: &CxxVector<u8>) -> FilterListMetadata;

        /// Enables a given tag for the engine.
        fn enable_tag(&mut self, tag: &CxxString);
        /// Disables a given tag for the engine.
        fn disable_tag(&mut self, tag: &CxxString);
        /// Returns true if a given tag is enabled for the engine.
        fn tag_exists(&self, key: &CxxString) -> bool;
        /// Checks if a given request should be blocked and returns an
        /// evaluation result struct with information on a matching rule
        /// and actions.
        fn matches(
            &self,
            url: &CxxString,
            hostname: &CxxString,
            source_hostname: &CxxString,
            request_type: &CxxString,
            third_party_request: bool,
            previously_matched_rule: bool,
            force_check_exceptions: bool,
        ) -> BlockerResult;
        /// Returns additional CSP directives to be added to a web response,
        /// if applicable to the request.
        fn get_csp_directives(
            &self,
            url: &CxxString,
            hostname: &CxxString,
            source_hostname: &CxxString,
            request_type: &CxxString,
            third_party_request: bool,
        ) -> String;
        pub fn serialize(&self) -> Vec<u8>;
        /// Deserializes and loads a binary-serialized Engine.
        fn deserialize(&mut self, serialized: &CxxVector<u8>) -> bool;
        /// Adds a resource to the engine resource set.
        fn add_resource(
            &mut self,
            name: &CxxString,
            content_type: &CxxString,
            data: &CxxString,
        ) -> UnitResult;
        /// Loads JSON-serialized resources into the engine resource set.
        fn use_resources(&mut self, resources_json: &CxxString) -> bool;
        /// Returns JSON-serialized cosmetic filter resources for a given url.
        fn url_cosmetic_resources(&self, url: &CxxString) -> String;

        /// Returns list of CSS selectors that require a generic CSS hide rule,
        /// from a given set of classes, ids and exceptions.
        fn hidden_class_id_selectors(
            &self,
            classes: &CxxVector<CxxString>,
            ids: &CxxVector<CxxString>,
            exceptions: &CxxVector<CxxString>,
        ) -> VecStringResult;
        /// Returns the blocker debug info containing regex info.
        fn get_regex_debug_info(&self) -> RegexDebugInfo;
        /// Removes a regex entry by the id.
        fn discard_regex(&mut self, regex_id: u64);
        /// Sets a discard policy for the regex manager.
        fn set_regex_discard_policy(&mut self, new_discard_policy: &RegexManagerDiscardPolicy);

        /// Converts a list in adblock syntax to its corresponding iOS
        /// content-blocking syntax. `truncated` will be set to indicate
        /// whether or not some rules had to be removed to avoid iOS's
        /// maximum rule count limit.
        fn convert_rules_to_content_blocking(rules: &CxxString) -> ContentBlockingRulesResult;
    }

    unsafe extern "C++" {
        include!("brave/components/brave_shields/adblock/resolver/adblock_domain_resolver.h");

        /// Wrapper function for
        /// net::registry_controlled_domains::GetDomainAndRegistry
        /// implemented in Chromium.
        fn resolve_domain_position(host: &CxxString) -> DomainPosition;
    }

    struct DomainPosition {
        start: u32,
        end: u32,
    }

    #[derive(Default)]
    struct BlockerResult {
        matched: bool,
        important: bool,
        has_exception: bool,
        redirect: OptionalString,
        rewritten_url: OptionalString,
        filter: OptionalString,
    }

    struct RegexDebugEntry {
        id: u64,
        regex: OptionalString,
        unused_secs: u64,
        usage_count: usize,
    }

    struct RegexDebugInfo {
        regex_data: Vec<RegexDebugEntry>,
        compiled_regex_count: usize,
    }

    struct RegexManagerDiscardPolicy {
        cleanup_interval_secs: u64,
        discard_unused_secs: u64,
    }

    #[derive(Default)]
    struct FilterListMetadata {
        homepage: OptionalString,
        title: OptionalString,
        expires_hours: OptionalU16,
    }

    #[derive(Default)]
    struct ContentBlockingRules {
        rules_json: String,
        truncated: bool,
    }

    enum ResultKind {
        Success,
        JsonError,
        Utf8Error,
        AdblockError,
    }

    // The following structs are not generic because generic type parameters are
    // not yet supported in cxx.
    // Created custom Result structs because cxx auto converts Result<T> to
    // std::exception, and exceptions are not allowed in Chromium.

    // To check a result for success/error, evaluate the result_kind ==
    // ResultKind::Success. If the condition is false, the result_kind will
    // contain an enum value describing the type of error, and error_message may
    // contain further details about the error.
    struct UnitResult {
        result_kind: ResultKind,
        error_message: String,
    }

    struct ContentBlockingRulesResult {
        value: ContentBlockingRules,
        result_kind: ResultKind,
        error_message: String,
    }

    struct VecStringResult {
        value: Vec<String>,
        result_kind: ResultKind,
        error_message: String,
    }

    struct BoxEngineResult {
        value: Box<Engine>,
        result_kind: ResultKind,
        error_message: String,
    }

    struct FilterListMetadataResult {
        value: FilterListMetadata,
        result_kind: ResultKind,
        error_message: String,
    }

    // Created custom Option struct because automatic conversion of Option<T>
    // is not yet supported in cxx.
    #[derive(Default)]
    struct OptionalString {
        has_value: bool,
        value: String,
    }

    #[derive(Default)]
    struct OptionalU16 {
        has_value: bool,
        value: u16,
    }
}
