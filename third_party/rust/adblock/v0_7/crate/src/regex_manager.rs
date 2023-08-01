//! A manager that creates/stores all regular expressions used by filters.
//! Rarely used entries could be discarded to save memory.
//! Non thread safe, the access must be synchronized externally.

use crate::filters::network::{compile_regex, CompiledRegex, NetworkFilter};

use std::collections::HashMap;
use std::time::Duration;

#[cfg(test)]
#[cfg(not(target_arch = "wasm32"))]
use mock_instant::Instant;
#[cfg(not(test))]
#[cfg(not(target_arch = "wasm32"))]
use std::time::Instant;

#[cfg(target_arch = "wasm32")]
#[derive(Clone, Copy)]
pub struct Instant;
#[cfg(target_arch = "wasm32")]
impl Instant {
    pub fn now() -> Self {
        Self
    }
}

/// `*const NetworkFilter` could technically leak across threads through `RegexDebugEntry::id`, but
/// it's disguised as a unique identifier and not intended to be dereferenced.
unsafe impl Send for RegexManager {}

const DEFAULT_CLEAN_UP_INTERVAL: Duration = Duration::from_secs(30);
const DEFAULT_DISCARD_UNUSED_TIME: Duration = Duration::from_secs(180);

pub struct RegexDebugEntry {
    pub id: u64,
    pub regex: Option<String>,
    pub last_used: Instant,
    pub usage_count: usize,
}

struct RegexEntry {
    regex: Option<CompiledRegex>,
    last_used: Instant,
    usage_count: usize,
}

pub struct RegexManagerDiscardPolicy {
    pub cleanup_interval: Duration,
    pub discard_unused_time: Duration,
}

impl Default for RegexManagerDiscardPolicy {
    fn default() -> Self {
        Self {
            cleanup_interval: DEFAULT_CLEAN_UP_INTERVAL,
            discard_unused_time: DEFAULT_DISCARD_UNUSED_TIME,
        }
    }
}

type RandomState = std::hash::BuildHasherDefault<seahash::SeaHasher>;

pub struct RegexManager {
    map: HashMap<*const NetworkFilter, RegexEntry, RandomState>,
    compiled_regex_count: usize,
    now: Instant,
    last_cleanup: Instant,
    discard_policy: RegexManagerDiscardPolicy,
}

impl Default for RegexManager {
    fn default() -> Self {
        Self {
            map: Default::default(),
            compiled_regex_count: 0,
            now: Instant::now(),
            last_cleanup: Instant::now(),
            discard_policy: Default::default(),
        }
    }
}

fn make_regexp(filter: &NetworkFilter) -> CompiledRegex {
    compile_regex(
        &filter.filter,
        filter.is_right_anchor(),
        filter.is_left_anchor(),
        filter.is_complete_regex(),
    )
}

impl RegexManager {
    pub fn matches(&mut self, filter: &NetworkFilter, pattern: &str) -> bool {
        if !filter.is_regex() && !filter.is_complete_regex() {
            return true;
        }
        let key = filter as *const NetworkFilter;
        use std::collections::hash_map::Entry;
        match self.map.entry(key) {
            Entry::Occupied(mut e) => {
                let v = e.get_mut();
                v.usage_count += 1;
                v.last_used = self.now;
                if v.regex.is_none() {
                    // A discarded entry, recreate it:
                    v.regex = Some(make_regexp(filter));
                    self.compiled_regex_count += 1;
                }
                return v.regex.as_ref().unwrap().is_match(pattern);
            }
            Entry::Vacant(e) => {
                self.compiled_regex_count += 1;
                let new_entry = RegexEntry {
                    regex: Some(make_regexp(filter)),
                    last_used: self.now,
                    usage_count: 1,
                };
                return e
                    .insert(new_entry)
                    .regex
                    .as_ref()
                    .unwrap()
                    .is_match(pattern);
            }
        };
    }

    #[cfg(not(target_arch = "wasm32"))]
    pub fn update_time(&mut self) {
        self.now = Instant::now();
        if !self.discard_policy.cleanup_interval.is_zero()
            && self.now - self.last_cleanup >= self.discard_policy.cleanup_interval
        {
            self.last_cleanup = self.now;
            self.cleanup();
        }
    }

    #[cfg(not(target_arch = "wasm32"))]
    pub fn cleanup(&mut self) {
        let now = self.now;
        for v in self.map.values_mut() {
            if now - v.last_used >= self.discard_policy.discard_unused_time {
                // Discard the regex to save memory.
                v.regex = None;
            }
        }
    }

    pub fn set_discard_policy(&mut self, new_discard_policy: RegexManagerDiscardPolicy) {
        self.discard_policy = new_discard_policy;
    }

    #[cfg(feature = "debug-info")]
    pub fn discard_regex(&mut self, regex_id: u64) {
        self.map
            .iter_mut()
            .filter(|(k, _)| **k as u64 == regex_id)
            .for_each(|(_, v)| {
                v.regex = None;
            });
    }

    #[cfg(feature = "debug-info")]
    pub fn get_debug_regex_data(&self) -> Vec<RegexDebugEntry> {
        use itertools::Itertools;
        self.map
            .iter()
            .map(|(k, e)| RegexDebugEntry {
                id: *k as u64,
                regex: e.regex.as_ref().map(|x| x.to_string()),
                last_used: e.last_used,
                usage_count: e.usage_count,
            })
            .collect_vec()
    }

    #[cfg(feature = "debug-info")]
    pub fn get_compiled_regex_count(&self) -> usize {
        self.compiled_regex_count
    }
}

#[cfg(all(test, feature = "debug-info"))]
mod tests {
    use super::*;

    use crate::filters::network::NetworkMatchable;
    use crate::request;

    use mock_instant::MockClock;

    fn make_filter(line: &str) -> NetworkFilter {
        NetworkFilter::parse(line, true, Default::default()).unwrap()
    }

    fn make_request(url: &str) -> request::Request {
        request::Request::from_url(url).unwrap()
    }

    fn get_active_regex_count(regex_manager: &RegexManager) -> usize {
        regex_manager
            .get_debug_regex_data()
            .iter()
            .filter(|x| x.regex.is_some())
            .count()
    }

    #[test]
    fn simple_match() {
        let mut regex_manager = RegexManager::default();
        regex_manager.update_time();

        let filter = make_filter("||geo*.hltv.org^");
        assert!(filter.matches(&make_request("https://geo2.hltv.org/"), &mut regex_manager));
        assert_eq!(get_active_regex_count(&regex_manager), 1);
        assert_eq!(regex_manager.get_debug_regex_data().len(), 1);
    }

    #[test]
    fn discard_and_recreate() {
        let mut regex_manager = RegexManager::default();
        regex_manager.update_time();

        let filter = make_filter("||geo*.hltv.org^");
        assert!(filter.matches(&make_request("https://geo2.hltv.org/"), &mut regex_manager));
        assert_eq!(regex_manager.get_compiled_regex_count(), 1);
        assert_eq!(get_active_regex_count(&regex_manager), 1);

        MockClock::advance(DEFAULT_DISCARD_UNUSED_TIME - Duration::from_secs(1));
        regex_manager.update_time();
        // The entry shouldn't be discarded because was used during
        // last REGEX_MANAGER_DISCARD_TIME.
        assert_eq!(get_active_regex_count(&regex_manager), 1);

        // The entry is entry is outdated, but should be discarded only
        // in the next cleanup() call. The call was 2 sec ago and is throttled
        // now.
        MockClock::advance(DEFAULT_CLEAN_UP_INTERVAL - Duration::from_secs(1));
        regex_manager.update_time();
        assert_eq!(get_active_regex_count(&regex_manager), 1);

        MockClock::advance(Duration::from_secs(2));
        regex_manager.update_time();
        // The entry is now outdated & cleanup() should be called => discard.
        assert_eq!(get_active_regex_count(&regex_manager), 0);

        // The entry is recreated, get_compiled_regex_count() increased +1.
        assert!(filter.matches(&make_request("https://geo2.hltv.org/"), &mut regex_manager));
        assert_eq!(regex_manager.get_compiled_regex_count(), 2);
        assert_eq!(get_active_regex_count(&regex_manager), 1);
    }
}
