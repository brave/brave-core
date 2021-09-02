use adblock::engine::Engine;
use adblock::resources::{Resource, ResourceType, MimeType};
use core::ptr;
use libc::size_t;
use std::ffi::CStr;
use std::ffi::CString;
use std::os::raw::c_char;
use std::string::String;

/// An external callback that receives a hostname and two out-parameters for start and end
/// position. The callback should fill the start and end positions with the start and end indices
/// of the domain part of the hostname.
pub type DomainResolverCallback = unsafe extern "C" fn(*const c_char, *mut u32, *mut u32);

/// Passes a callback to the adblock library, allowing it to be used for domain resolution.
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

    adblock::url_parser::set_domain_resolver(Box::new(RemoteResolverImpl { remote_callback: resolver })).is_ok()
}

/// Create a new `Engine`.
#[no_mangle]
pub unsafe extern "C" fn engine_create_from_buffer(
    data: *const c_char,
    data_size: size_t,
) -> *mut Engine {
    let data: &[u8] = std::slice::from_raw_parts(data as *const u8, data_size);
    let rules = std::str::from_utf8(data).unwrap_or_else(|_| {
        eprintln!("Failed to parse filter list with invalid UTF-8 content");
        ""
    });
    engine_create_from_str(rules)
}

#[no_mangle]
pub unsafe extern "C" fn engine_create(rules: *const c_char) -> *mut Engine {
    let rules = CStr::from_ptr(rules).to_str().unwrap_or("");
    engine_create_from_str(rules)
}

fn engine_create_from_str(rules: &str) -> *mut Engine {
    let mut filter_set = adblock::lists::FilterSet::new(false);
    filter_set.add_filter_list(&rules, adblock::lists::FilterFormat::Standard);
    let engine = Engine::from_filter_set(filter_set, true);
    Box::into_raw(Box::new(engine))
}

/// Checks if a `url` matches for the specified `Engine` within the context.
///
/// This API is designed for multi-engine use, so block results are used both as inputs and
/// outputs. They will be updated to reflect additional checking within this engine, rather than
/// being replaced with results just for this engine.
#[no_mangle]
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
    *redirect = match blocker_result.redirect {
        Some(x) => match CString::new(x) {
            Ok(y) => y.into_raw(),
            _ => ptr::null_mut(),
        },
        None => ptr::null_mut(),
    };
}

/// Returns any CSP directives that should be added to a subdocument or document request's response
/// headers.
#[no_mangle]
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
    if let Some(directive) = engine.get_csp_directives(url, host, tab_host, resource_type, Some(third_party)) {
        let ptr = CString::new(directive)
            .expect("Error: CString::new()")
            .into_raw();
        std::mem::forget(ptr);
        ptr
    } else {
        let ptr = CString::new("")
            .expect("Error: CString::new()")
            .into_raw();
        std::mem::forget(ptr);
        ptr
    }
}

/// Adds a tag to the engine for consideration
#[no_mangle]
pub unsafe extern "C" fn engine_add_tag(engine: *mut Engine, tag: *const c_char) {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.enable_tags(&[tag]);
}

/// Checks if a tag exists in the engine
#[no_mangle]
pub unsafe extern "C" fn engine_tag_exists(engine: *mut Engine, tag: *const c_char) -> bool {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.tag_exists(tag)
}

/// Adds a resource to the engine by name
#[no_mangle]
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

/// Adds a list of `Resource`s from JSON format
#[no_mangle]
pub unsafe extern "C" fn engine_add_resources(engine: *mut Engine, resources: *const c_char) {
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
pub unsafe extern "C" fn engine_remove_tag(engine: *mut Engine, tag: *const c_char) {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.disable_tags(&[tag]);
}

/// Deserializes a previously serialized data file list.
#[no_mangle]
pub unsafe extern "C" fn engine_deserialize(
    engine: *mut Engine,
    data: *const c_char,
    data_size: size_t,
) -> bool {
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
pub unsafe extern "C" fn engine_destroy(engine: *mut Engine) {
    if !engine.is_null() {
        drop(Box::from_raw(engine));
    }
}

/// Destroy a `*c_char` once you are done with it.
#[no_mangle]
pub unsafe extern "C" fn c_char_buffer_destroy(s: *mut c_char) {
    if !s.is_null() {
        drop(CString::from_raw(s));
    }
}

/// Returns a set of cosmetic filtering resources specific to the given url, in JSON format
#[no_mangle]
pub unsafe extern "C" fn engine_url_cosmetic_resources(
    engine: *mut Engine,
    url: *const c_char,
) -> *mut c_char {
    let url = CStr::from_ptr(url).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    let ptr = CString::new(serde_json::to_string(&engine.url_cosmetic_resources(url))
        .unwrap_or_else(|_| "".into()))
        .expect("Error: CString::new()")
        .into_raw();
    std::mem::forget(ptr);
    ptr
}

/// Returns a stylesheet containing all generic cosmetic rules that begin with any of the provided class and id selectors
///
/// The leading '.' or '#' character should not be provided
#[no_mangle]
pub unsafe extern "C" fn engine_hidden_class_id_selectors(
    engine: *mut Engine,
    classes: *const *const c_char,
    classes_size: size_t,
    ids: *const *const c_char,
    ids_size: size_t,
    exceptions: *const *const c_char,
    exceptions_size: size_t,
) -> *mut c_char {
    let classes = std::slice::from_raw_parts(classes, classes_size);
    let classes: Vec<String> = (0..classes_size)
        .map(|index| CStr::from_ptr(classes[index]).to_str().unwrap().to_owned())
        .collect();
    let ids = std::slice::from_raw_parts(ids, ids_size);
    let ids: Vec<String> = (0..ids_size)
        .map(|index| CStr::from_ptr(ids[index]).to_str().unwrap().to_owned())
        .collect();
    let exceptions = std::slice::from_raw_parts(exceptions, exceptions_size);
    let exceptions: std::collections::HashSet<String> = (0..exceptions_size)
        .map(|index| CStr::from_ptr(exceptions[index]).to_str().unwrap().to_owned())
        .collect();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    let stylesheet = engine.hidden_class_id_selectors(&classes, &ids, &exceptions);
    CString::new(serde_json::to_string(&stylesheet).unwrap_or_else(|_| "".into())).expect("Error: CString::new()").into_raw()
}
