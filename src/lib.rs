extern crate adblock;

use adblock::engine::Engine;
use std::ffi::CStr;
use std::os::raw::c_char;
use std::string::String;

/// Create a new `Engine`.
#[no_mangle]
pub unsafe extern "C" fn engine_create(rules: *const c_char) -> *mut Engine {
    let split = CStr::from_ptr(rules).to_str().unwrap().lines();
    let rules: Vec<String> = split.map(String::from).collect();
    let engine = Engine::from_rules(rules.as_slice());
    Box::into_raw(Box::new(engine))
}

/// Checks if a `url` matches for the specified `Engine` within the context.
#[no_mangle]
pub unsafe extern "C" fn engine_match(
    engine: *mut Engine,
    url: *const c_char,
    host: *const c_char,
    tab_host: *const c_char,
    third_party: bool,
    resource_type: *const c_char,
) -> bool {
    let url = CStr::from_ptr(url).to_str().unwrap();
    let host = CStr::from_ptr(host).to_str().unwrap();
    let tab_host = CStr::from_ptr(tab_host).to_str().unwrap();
    let resource_type = CStr::from_ptr(resource_type).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine
        .check_network_urls_with_hostnames(url, host, tab_host, resource_type, Some(third_party))
        .matched
}

/// Adds a tag to the engine for consideration
#[no_mangle]
pub unsafe extern "C" fn engine_add_tag(
    engine: *mut Engine,
    tag: *const c_char,
) {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine
        .tags_enable(&[tag]);
}


/// Removes a tag to the engine for consideration
#[no_mangle]
pub unsafe extern "C" fn engine_remove_tag(
    engine: *mut Engine,
    tag: *const c_char,
) {
    let tag = CStr::from_ptr(tag).to_str().unwrap();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine
        .tags_disable(&[tag]);
}

/// Deserializes a previously serialized data file list.
#[no_mangle]
pub unsafe extern "C" fn engine_deserialize(engine: *mut Engine, data: *const c_char) -> bool {
    let data = CStr::from_ptr(data).to_bytes();
    assert!(!engine.is_null());
    let engine = Box::leak(Box::from_raw(engine));
    engine.deserialize(&data).is_ok()
}

/// Destroy a `Engine` once you are done with it.
#[no_mangle]
pub unsafe extern "C" fn engine_destroy(engine: *mut Engine) {
    if !engine.is_null() {
        drop(Box::from_raw(engine));
    }
}
