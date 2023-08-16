/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use adblock::engine::{Engine, EngineDebugInfo};
use adblock::lists::FilterListMetadata;
use adblock::regex_manager::RegexManagerDiscardPolicy;
use adblock::resources::{MimeType, Resource, ResourceType};
use core::ptr;
use libc::size_t;
use std::collections::HashSet;
use std::ffi::CStr;
use std::ffi::CString;
use std::os::raw::c_char;

/// An external callback that receives a hostname and two out-parameters for
/// start and end position. The callback should fill the start and end positions
/// with the start and end indices of the domain part of the hostname.
pub type DomainResolverCallback = unsafe extern "C" fn(*const c_char, *mut u32, *mut u32);

/// Passes a callback to the adblock library, allowing it to be used for domain
/// resolution.
///
/// This is required to be able to use any adblocking functionality.
///
/// Returns true on success, false if a callback was already set previously.
#[no_mangle]
pub unsafe extern "C" fn set_domain_resolver(resolver: DomainResolverCallback) -> bool {
    struct RemoteResolverImpl {
        remote_callback: DomainResolverCallback,
    }

    impl adblock::url_parser::ResolvesDomain for RemoteResolverImpl {
        fn get_host_domain(&self, host: &str) -> (usize, usize) {
            let mut start: u32 = 0;
            let mut end: u32 = 0;
            let host_c_str = CString::new(host).expect("Error: CString::new()");
            let remote_callback = self.remote_callback;

            unsafe {
                remote_callback(host_c_str.as_ptr(), &mut start as *mut u32, &mut end as *mut u32);
            }

            (start as usize, end as usize)
        }
    }

    adblock::url_parser::set_domain_resolver(Box::new(RemoteResolverImpl {
        remote_callback: resolver,
    }))
    .is_ok()
}

/// Create a new `Engine`, interpreting `data` as a C string and then parsing as
/// a filter list in ABP syntax.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_create_from_buffer(
    data: *const c_char,
    data_size: size_t,
) -> *mut Engine {
    if data_size == 0 {
        Box::into_raw(Box::new(Engine::default()))
    } else {
        let data: &[u8] = std::slice::from_raw_parts(data as *const u8, data_size);
        let rules = std::str::from_utf8(data).unwrap_or_else(|_| {
            eprintln!("Failed to parse filter list with invalid UTF-8 content");
            ""
        });
        engine_create_from_str(rules).1
    }
}

/// Create a new `Engine`, interpreting `data` as a C string and then parsing as
/// a filter list in ABP syntax. Also populates metadata from the filter list
/// into `metadata`.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_create_from_buffer_with_metadata(
    data: *const c_char,
    data_size: size_t,
    metadata: *mut *mut FilterListMetadata,
) -> *mut Engine {
    let (metadata_ptr, engine_ptr) = if data_size == 0 {
        let metadata = FilterListMetadata::default();
        let engine = Engine::default();
        (
            Box::into_raw(Box::new(metadata)),
            Box::into_raw(Box::new(engine)),
        )
    } else {
        let data: &[u8] = std::slice::from_raw_parts(data as *const u8, data_size);
        let rules = std::str::from_utf8(data).unwrap_or_else(|_| {
            eprintln!("Failed to parse filter list with invalid UTF-8 content");
            ""
        });
        engine_create_from_str(rules)
    };
    *metadata = metadata_ptr;
    engine_ptr
}

/// Create a new `Engine`, interpreting `rules` as a null-terminated C string
/// and then parsing as a filter list in ABP syntax.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_create(rules: *const c_char) -> *mut Engine {
    let rules = CStr::from_ptr(rules).to_str().unwrap_or_else(|_| {
        eprintln!("Failed to parse filter list with invalid UTF-8 content");
        ""
    });
    engine_create_from_str(rules).1
}

/// Create a new `Engine`, interpreting `rules` as a null-terminated C string
/// and then parsing as a filter list in ABP syntax. Also populates metadata
/// from the filter list into `metadata`.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_create_with_metadata(
    rules: *const c_char,
    metadata: *mut *mut FilterListMetadata,
) -> *mut Engine {
    let rules = CStr::from_ptr(rules).to_str().unwrap_or_else(|_| {
        eprintln!("Failed to parse filter list with invalid UTF-8 content");
        ""
    });
    let (metadata_ptr, engine_ptr) = engine_create_from_str(rules);
    *metadata = metadata_ptr;
    engine_ptr
}

/// Scans the beginning of the list for metadata and returns it without parsing
/// any other list content.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn read_list_metadata(
    data: *const c_char,
    data_size: size_t,
) -> *mut FilterListMetadata {
    let metadata = if data_size == 0 {
        adblock::lists::FilterListMetadata::default()
    } else {
        let data: &[u8] = std::slice::from_raw_parts(data as *const u8, data_size);
        let list = std::str::from_utf8(data).unwrap_or_else(|_| {
            eprintln!("Failed to parse filter list with invalid UTF-8 content");
            ""
        });
        adblock::lists::read_list_metadata(list)
    };
    Box::into_raw(Box::new(metadata))
}

fn engine_create_from_str(rules: &str) -> (*mut FilterListMetadata, *mut Engine) {
    let mut filter_set = adblock::lists::FilterSet::new(false);
    let metadata = filter_set.add_filter_list(&rules, Default::default());
    let engine = Engine::from_filter_set(filter_set, true);
    (Box::into_raw(Box::new(metadata)), Box::into_raw(Box::new(engine)))
}

/// Checks if a `url` matches for the specified `Engine` within the context.
///
/// This API is designed for multi-engine use, so block results are used both as
/// inputs and outputs. They will be updated to reflect additional checking
/// within this engine, rather than being replaced with results just for this
/// engine.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_match(
    engine: *mut Engine,
    url: *const c_char,
    host: *const c_char,
    tab_host: *const c_char,
    third_party: bool,
    resource_type: *const c_char,
    did_match_rule: *mut bool,
    did_match_exception: *mut bool,
    did_match_important: *mut bool,
    redirect: *mut *mut c_char,
    rewritten_url: *mut *mut c_char,
) {
    let url = CStr::from_ptr(url).to_str().unwrap();
    let host = CStr::from_ptr(host).to_str().unwrap();
    let tab_host = CStr::from_ptr(tab_host).to_str().unwrap();
    let resource_type = CStr::from_ptr(resource_type).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    let blocker_result = engine.check_network_urls_with_hostnames_subset(
        url,
        host,
        tab_host,
        resource_type,
        Some(third_party),
        // Checking normal rules is skipped if a normal rule or exception rule was found previously
        *did_match_rule || *did_match_exception,
        // Always check exceptions unless one was found previously
        !*did_match_exception,
    );
    *did_match_rule |= blocker_result.matched;
    *did_match_exception |= blocker_result.exception.is_some();
    *did_match_important |= blocker_result.important;
    *redirect = blocker_result
        .redirect
        .and_then(|x| CString::new(x).map(CString::into_raw).ok())
        .unwrap_or(ptr::null_mut());
    *rewritten_url = blocker_result
        .rewritten_url
        .and_then(|x| CString::new(x).map(CString::into_raw).ok())
        .unwrap_or(ptr::null_mut());
}

/// Returns any CSP directives that should be added to a subdocument or document
/// request's response headers.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_get_csp_directives(
    engine: *mut Engine,
    url: *const c_char,
    host: *const c_char,
    tab_host: *const c_char,
    third_party: bool,
    resource_type: *const c_char,
) -> *mut c_char {
    let url = CStr::from_ptr(url).to_str().unwrap();
    let host = CStr::from_ptr(host).to_str().unwrap();
    let tab_host = CStr::from_ptr(tab_host).to_str().unwrap();
    let resource_type = CStr::from_ptr(resource_type).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    if let Some(directive) =
        engine.get_csp_directives(url, host, tab_host, resource_type, Some(third_party))
    {
        CString::new(directive).expect("Error: CString::new()").into_raw()
    } else {
        CString::new("").expect("Error: CString::new()").into_raw()
    }
}

/// Adds a tag to the engine for consideration
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_add_tag(engine: *mut Engine, tag: *const c_char) {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.enable_tags(&[tag]);
}

/// Checks if a tag exists in the engine
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_tag_exists(engine: *mut Engine, tag: *const c_char) -> bool {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.tag_exists(tag)
}

/// Adds a resource to the engine by name
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_add_resource(
    engine: *mut Engine,
    key: *const c_char,
    content_type: *const c_char,
    data: *const c_char,
) -> bool {
    let key = CStr::from_ptr(key).to_str().unwrap();
    let content_type = CStr::from_ptr(content_type).to_str().unwrap();
    let data = CStr::from_ptr(data).to_str().unwrap();
    let resource = Resource {
        name: key.to_string(),
        aliases: vec![],
        kind: ResourceType::Mime(MimeType::from(std::borrow::Cow::from(content_type))),
        content: data.to_string(),
    };
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.add_resource(resource).is_ok()
}

/// Uses a list of `Resource`s from JSON format
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_use_resources(engine: *mut Engine, resources: *const c_char) {
    let resources = CStr::from_ptr(resources).to_str().unwrap();
    let resources: Vec<Resource> = serde_json::from_str(resources).unwrap_or_else(|e| {
        eprintln!("Failed to parse JSON adblock resources: {}", e);
        vec![]
    });
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.use_resources(&resources);
}

/// Removes a tag to the engine for consideration
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_remove_tag(engine: *mut Engine, tag: *const c_char) {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.disable_tags(&[tag]);
}

/// Deserializes a previously serialized data file list.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_deserialize(
    engine: *mut Engine,
    data: *const c_char,
    data_size: size_t,
) -> bool {
    if data_size == 0 {
        return true;
    }
    let data: &[u8] = std::slice::from_raw_parts(data as *const u8, data_size);
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    let ok = engine.deserialize(&data).is_ok();
    if !ok {
        eprintln!("Error deserializing adblock engine");
    }
    ok
}

/// Destroy a `Engine` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_destroy(engine: *mut Engine) {
    if !engine.is_null() {
        drop(Box::from_raw(engine));
    }
}

/// Puts a pointer to the homepage of the `FilterListMetadata` into `homepage`.
/// Returns `true` if a homepage was returned.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn filter_list_metadata_homepage(
    metadata: *const FilterListMetadata,
    homepage: *mut *mut c_char,
) -> bool {
    if let Some(this_homepage) = (*metadata).homepage.as_ref() {
        let cstring = CString::new(this_homepage.as_str());
        match cstring {
            Ok(cstring) => {
                *homepage = cstring.into_raw();
                true
            }
            Err(_) => false,
        }
    } else {
        false
    }
}

/// Puts a pointer to the title of the `FilterListMetadata` into `title`.
/// Returns `true` if a title was returned.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn filter_list_metadata_title(
    metadata: *const FilterListMetadata,
    title: *mut *mut c_char,
) -> bool {
    if let Some(this_title) = (*metadata).title.as_ref() {
        let cstring = CString::new(this_title.as_str());
        match cstring {
            Ok(cstring) => {
                *title = cstring.into_raw();
                true
            }
            Err(_) => false,
        }
    } else {
        false
    }
}

#[no_mangle]
pub static SUBSCRIPTION_DEFAULT_EXPIRES_HOURS: u16 = 7 * 24;

/// Returns the amount of time this filter list should be considered valid for,
/// in hours. Defaults to 168 (i.e. 7 days) if unspecified by the
/// `FilterListMetadata`.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn filter_list_metadata_expires(metadata: *const FilterListMetadata) -> u16 {
    use adblock::lists::ExpiresInterval;

    match (*metadata).expires.as_ref() {
        Some(ExpiresInterval::Days(d)) => 24 * *d as u16,
        Some(ExpiresInterval::Hours(h)) => *h,
        None => SUBSCRIPTION_DEFAULT_EXPIRES_HOURS,
    }
}

/// Destroy a `FilterListMetadata` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn filter_list_metadata_destroy(metadata: *mut FilterListMetadata) {
    if !metadata.is_null() {
        drop(Box::from_raw(metadata));
    }
}

/// Get EngineDebugInfo from the engine. Should be destoyed later by calling
/// engine_debug_info_destroy(..).
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn get_engine_debug_info(engine: *mut Engine) -> *mut EngineDebugInfo {
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    Box::into_raw(Box::new(engine.get_debug_info()))
}

/// Returns the field of EngineDebugInfo structure.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_debug_info_get_attr(
    debug_info: *mut EngineDebugInfo,
    compiled_regex_count: *mut size_t,
    regex_data_size: *mut size_t,
) {
    assert!(!debug_info.is_null());
    let info = Box::leak(Box::from_raw(debug_info));

    *compiled_regex_count = info.blocker_debug_info.compiled_regex_count;
    *regex_data_size = info.blocker_debug_info.regex_data.len();
}

/// Returns the fields of EngineDebugInfo->regex_data[index].
///
/// |regex| stay untouched if it ==None in the original structure.
///
/// |index| must be in range [0, regex_data.len() - 1].
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_debug_info_get_regex_entry(
    debug_info: *mut EngineDebugInfo,
    index: size_t,
    id: *mut u64,
    regex: *mut *mut c_char,
    unused_sec: *mut u64,
    usage_count: *mut usize,
) {
    assert!(!debug_info.is_null());
    let info = Box::leak(Box::from_raw(debug_info));
    let regex_data = &info.blocker_debug_info.regex_data;
    assert!(index < regex_data.len());
    let entry = &regex_data[index];

    *id = entry.id;
    *regex = CString::new(entry.regex.as_deref().unwrap_or(""))
        .expect("Error: CString::new()")
        .into_raw();
    *unused_sec = entry.last_used.elapsed().as_secs();
    *usage_count = entry.usage_count;
}

/// Destroy a `EngineDebugInfo` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_debug_info_destroy(debug_info: *mut EngineDebugInfo) {
    if !debug_info.is_null() {
        drop(Box::from_raw(debug_info));
    }
}

#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn discard_regex(engine: *mut Engine, regex_id: u64) {
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.discard_regex(regex_id);
}

/// Setup discard policy for adblock regexps.
///
/// |cleanup_interval_sec| how ofter the engine should check the policy.
///
/// |discard_unused_sec| time in sec after unused regex will be discarded. Zero
/// means disable discarding completely.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn setup_discard_policy(
    engine: *mut Engine,
    cleanup_interval_sec: u64,
    discard_unused_sec: u64,
) {
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.set_regex_discard_policy(RegexManagerDiscardPolicy {
        cleanup_interval: std::time::Duration::from_secs(cleanup_interval_sec),
        discard_unused_time: std::time::Duration::from_secs(discard_unused_sec),
    });
}

/// Destroy a `*c_char` once you are done with it.
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn c_char_buffer_destroy(s: *mut c_char) {
    if !s.is_null() {
        drop(CString::from_raw(s));
    }
}

/// Returns a set of cosmetic filtering resources specific to the given url, in
/// JSON format
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_url_cosmetic_resources(
    engine: *mut Engine,
    url: *const c_char,
) -> *mut c_char {
    let url = CStr::from_ptr(url).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    CString::new(
        serde_json::to_string(&engine.url_cosmetic_resources(url)).unwrap_or_else(|_| "{}".into()),
    )
    .expect("Error: CString::new()")
    .into_raw()
}

/// Returns a stylesheet containing all generic cosmetic rules that begin with
/// any of the provided class and id selectors
///
/// The leading '.' or '#' character should not be provided
#[no_mangle]
#[allow(unsafe_op_in_unsafe_fn)]
pub unsafe extern "C" fn engine_hidden_class_id_selectors(
    engine: *mut Engine,
    classes: *const *const c_char,
    classes_size: size_t,
    ids: *const *const c_char,
    ids_size: size_t,
    exceptions: *const *const c_char,
    exceptions_size: size_t,
) -> *mut c_char {
    // Note checks for `size == 0` - `std::vector<T>::data()`'s return value when
    // empty is undefined behavior and should never be used.
    let classes = if classes_size == 0 {
        Vec::new()
    } else {
        let classes = std::slice::from_raw_parts(classes, classes_size);
        (0..classes_size)
            .map(|index| CStr::from_ptr(classes[index]).to_str().unwrap().to_owned())
            .collect()
    };

    let ids = if ids_size == 0 {
        Vec::new()
    } else {
        let ids = std::slice::from_raw_parts(ids, ids_size);
        (0..ids_size).map(|index| CStr::from_ptr(ids[index]).to_str().unwrap().to_owned()).collect()
    };

    let exceptions = if exceptions_size == 0 {
        HashSet::new()
    } else {
        let exceptions = std::slice::from_raw_parts(exceptions, exceptions_size);
        (0..exceptions_size)
            .map(|index| CStr::from_ptr(exceptions[index]).to_str().unwrap().to_owned())
            .collect()
    };

    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    let stylesheet = engine.hidden_class_id_selectors(&classes, &ids, &exceptions);
    CString::new(serde_json::to_string(&stylesheet).unwrap_or_else(|_| "".into()))
        .expect("Error: CString::new()")
        .into_raw()
}

/// Converts a list in adblock syntax to its corresponding iOS content-blocking
/// syntax. `truncated` will be set to indicate whether or not some rules had to
/// be removed to avoid iOS's maximum rule count limit.
#[cfg(feature = "ios")]
#[allow(unsafe_op_in_unsafe_fn)]
#[no_mangle]
pub unsafe extern "C" fn convert_rules_to_content_blocking(
    rules: *const c_char,
    truncated: *mut bool,
) -> *mut c_char {
    use adblock::lists::{ParseOptions, RuleTypes};

    /// This value corresponds to `maxRuleCount` here:
    /// https://github.com/WebKit/WebKit/blob/4a2df13be2253f64d8da58b794d74347a3742652/Source/WebCore/contentextensions/ContentExtensionParser.cpp#L299
    const MAX_CB_LIST_SIZE: usize = 150000;

    let rules = CStr::from_ptr(rules).to_str().unwrap_or_else(|_| {
        eprintln!("Failed to parse filter list with invalid UTF-8 content");
        ""
    });
    let mut filter_set = adblock::lists::FilterSet::new(true);
    filter_set.add_filter_list(
        &rules,
        ParseOptions { rule_types: RuleTypes::NetworkOnly, ..Default::default() },
    );
    // `unwrap` is safe here because `into_content_blocking` only panics if the
    // `FilterSet` was not created in debug mode
    let (mut cb_rules, _) = filter_set.into_content_blocking().unwrap();
    let rules_len = cb_rules.len();
    if rules_len > MAX_CB_LIST_SIZE {
        // Note that the last rule is always the first-party document exception rule,
        // which we want to keep. Otherwise, we can arbitrarily truncate rules
        // before that to ensure that the list can actually compile.
        cb_rules.swap(rules_len - 1, MAX_CB_LIST_SIZE - 1);
        cb_rules.truncate(MAX_CB_LIST_SIZE);
        *truncated = true;
    } else {
        *truncated = false;
    }
    CString::new(serde_json::to_string(&cb_rules).unwrap_or_else(|_| "".into()))
        .expect("Error: CString::new()")
        .into_raw()
}
