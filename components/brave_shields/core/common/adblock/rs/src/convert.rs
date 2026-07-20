/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use std::time::Duration;

use crate::ffi::{
    AddedFiltersRecord, BlockerResult, DebugInfo, FilterListMetadata, FilterRuleInfo,
    OptionalString, OptionalU16, RegexDebugEntry, RegexManagerDiscardPolicy, SourceInfo,
};
use adblock::blocker::BlockerResult as InnerBlockerResult;
use adblock::engine::{EngineDebugInfo as InnerEngineDebugInfo, SourceInfo as InnerSourceInfo};
use adblock::lists::{
    AddedFiltersRecord as InnerAddedFiltersRecord, ExpiresInterval,
    FilterListMetadata as InnerFilterListMetadata,
};

use adblock::regex_manager::{
    RegexDebugEntry as InnerRegexDebugEntry,
    RegexManagerDiscardPolicy as InnerRegexManagerDiscardPolicy,
};
use adblock::sourcemap::FilterRuleDebugInfo as InnerFilterRuleDebugInfo;
use cxx::UniquePtr;

impl From<Option<String>> for OptionalString {
    fn from(value: Option<String>) -> Self {
        match value {
            None => Self { has_value: false, value: String::new() },
            Some(value) => Self { has_value: true, value },
        }
    }
}

impl From<Option<u16>> for OptionalU16 {
    fn from(value: Option<u16>) -> Self {
        match value {
            None => Self { has_value: false, value: 0 },
            Some(value) => Self { has_value: true, value },
        }
    }
}

impl From<InnerRegexDebugEntry> for RegexDebugEntry {
    fn from(entry: InnerRegexDebugEntry) -> Self {
        Self {
            id: entry.id,
            regex: OptionalString::from(entry.regex),
            unused_secs: entry.last_used.elapsed().as_secs(),
            usage_count: entry.usage_count,
        }
    }
}

impl From<InnerSourceInfo> for SourceInfo {
    fn from(info: InnerSourceInfo) -> Self {
        Self {
            title: OptionalString::from(info.title),
            homepage: OptionalString::from(info.homepage),
            network_filter_count: info.network_filter_count as usize,
            cosmetic_filter_count: info.cosmetic_filter_count as usize,
            parse_error: info.parse_error as usize,
            invalid_lines: info.invalid_lines,
        }
    }
}

impl From<InnerEngineDebugInfo> for DebugInfo {
    fn from(info: InnerEngineDebugInfo) -> Self {
        Self {
            regex_data: info.regex_debug_info.regex_data.into_iter().map(|e| e.into()).collect(),
            compiled_regex_count: info.regex_debug_info.compiled_regex_count,
            flatbuffer_size: info.flatbuffer_size,
            source_info: info.source_info.into_iter().map(|e| e.into()).collect(),
        }
    }
}

// Converts `Option<FilterRuleDebugInfo>` into a `UniquePtr<FilterRuleInfo>`.
// Null unless debug mode was enabled *and* the matched rule actually carries
// usable debug info.
//
// Note that `raw_line` is `Some("")` (not `None`) whenever debug mode is on,
// even for filters that were fused together by the optimizer and therefore
// have no meaningful source info (see `FlatNetworkFilter::raw_line` in
// adblock-rust), so it isn't enough to check `Option::is_some()` here.
fn to_filter_rule_info(info: Option<InnerFilterRuleDebugInfo>) -> UniquePtr<FilterRuleInfo> {
    let Some(info) = info else {
        return UniquePtr::null();
    };
    let has_raw_line = info.raw_line.as_deref().is_some_and(|line| !line.is_empty());
    if !has_raw_line && info.source_location.is_none() {
        return UniquePtr::null();
    }
    let source_location = info.source_location.unwrap_or_default();
    UniquePtr::new(FilterRuleInfo {
        raw_line: info.raw_line.unwrap_or_default(),
        source_index: source_location.source_index,
        line_number: source_location.line_number,
    })
}

impl From<InnerBlockerResult> for BlockerResult {
    fn from(result: InnerBlockerResult) -> Self {
        let matched = result.should_block();
        let has_exception = result.exception.is_some();
        Self {
            matched,
            important: result.important,
            has_exception,
            filter: to_filter_rule_info(result.filter),
            exception: to_filter_rule_info(result.exception),
            redirect: result.redirect.into(),
            rewritten_url: result.rewritten_url.into(),
        }
    }
}

impl From<&RegexManagerDiscardPolicy> for InnerRegexManagerDiscardPolicy {
    fn from(policy: &RegexManagerDiscardPolicy) -> Self {
        Self {
            cleanup_interval: Duration::from_secs(policy.cleanup_interval_secs),
            discard_unused_time: Duration::from_secs(policy.discard_unused_secs),
        }
    }
}

impl From<InnerFilterListMetadata> for FilterListMetadata {
    fn from(metadata: InnerFilterListMetadata) -> Self {
        let expires_hours = OptionalU16::from(metadata.expires.map(|interval| match interval {
            ExpiresInterval::Hours(hours) => hours,
            ExpiresInterval::Days(days) => days as u16 * 24,
        }));
        Self { homepage: metadata.homepage.into(), title: metadata.title.into(), expires_hours }
    }
}

impl From<InnerAddedFiltersRecord> for AddedFiltersRecord {
    fn from(record: InnerAddedFiltersRecord) -> Self {
        Self { source_index: record.source_index, metadata: record.metadata.into() }
    }
}
