/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use std::time::Duration;

use crate::ffi::{
    BlockerResult, FilterListMetadata, OptionalString, OptionalU16, RegexDebugEntry,
    RegexDebugInfo, RegexManagerDiscardPolicy,
};
use adblock::blocker::BlockerResult as InnerBlockerResult;
use adblock::lists::{ExpiresInterval, FilterListMetadata as InnerFilterListMetadata};
use adblock::regex_manager::{
    RegexDebugEntry as InnerRegexDebugEntry,
    RegexDebugInfo as InnerRegexDebugInfo,
    RegexManagerDiscardPolicy as InnerRegexManagerDiscardPolicy,
};

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

impl From<InnerRegexDebugInfo> for RegexDebugInfo {
    fn from(info: InnerRegexDebugInfo) -> Self {
        Self {
            regex_data: info.regex_data.into_iter().map(|e| e.into()).collect(),
            compiled_regex_count: info.compiled_regex_count,
        }
    }
}

impl From<InnerBlockerResult> for BlockerResult {
    fn from(result: InnerBlockerResult) -> Self {
        Self {
            matched: result.matched,
            important: result.important,
            has_exception: result.exception.is_some(),
            redirect: result.redirect.into(),
            rewritten_url: result.rewritten_url.into(),
            filter: result.filter.into(),
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
