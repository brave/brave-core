extern crate adblock;

use adblock::blocker::{Blocker, BlockerOptions};
use adblock::lists::parse_filters;
use adblock::request::Request;
use std::ffi::CStr;
use std::os::raw::c_char;
use std::string::String;

/// Create a new `Blocker`.
#[no_mangle]
pub unsafe extern "C" fn blocker_create(rules: *const c_char) -> *mut Blocker {
    let split = CStr::from_ptr(rules).to_str().unwrap().lines();
    let rules: Vec<String> = split.map(|s| String::from(s)).collect();
    let (network_filters, _) = parse_filters(&rules, true, false, false);
    let blocker_options = BlockerOptions {
        debug: false,
        enable_optimizations: false,
        load_cosmetic_filters: false,
        load_network_filters: true,
    };
    let blocker = Blocker::new(network_filters, &blocker_options);
    Box::into_raw(Box::new(blocker))
}

/// Checks if a `url` matches for the specified `Blocker` within the context.
#[no_mangle]
pub unsafe extern "C" fn blocker_match(
    blocker: *mut Blocker,
    url: *const c_char,
    tab_url: *const c_char,
    resource_type: *const c_char,
) -> bool {
    let url = CStr::from_ptr(url).to_str().unwrap();
    let tab_url = CStr::from_ptr(tab_url).to_str().unwrap();
    let resource_type = CStr::from_ptr(resource_type).to_str().unwrap();

    assert!(!blocker.is_null());
    let maybe_req = Request::from_urls(url, tab_url, resource_type);
    assert!(maybe_req.is_ok(), "Request failed to parse");
    let req = maybe_req.unwrap();
    Box::leak(Box::from_raw(blocker)).check(&req).matched
}

/// Destroy a `Blocker` once you are done with it.
#[no_mangle]
pub unsafe extern "C" fn blocker_destroy(blocker: *mut Blocker) {
    if !blocker.is_null() {
        drop(Box::from_raw(blocker));
    }
}
