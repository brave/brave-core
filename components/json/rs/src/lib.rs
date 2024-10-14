// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = rust_json)]
mod ffi {
    extern "Rust" {
        fn convert_uint64_value_to_string(path: &str, json: &str, optional: bool) -> String;
        fn convert_int64_value_to_string(path: &str, json: &str, optional: bool) -> String;
        fn convert_string_value_to_uint64(path: &str, json: &str, optional: bool) -> String;
        fn convert_string_value_to_int64(path: &str, json: &str, optional: bool) -> String;
        fn convert_uint64_in_object_array_to_string(
            path_to_list: &str,
            path_to_object: &str,
            key: &str,
            json: &str,
        ) -> String;
        fn convert_all_numbers_to_string(json: &str, path: &str) -> String;
        fn convert_all_numbers_to_string_and_remove_null_values(json: &str, path: &str) -> String;
    }
}

// Parses and re-serializes json with the value at path converted from a uint64
// to a string representation of the same number.
// Returns an empty String if such conversion is not possible.
// When optional is true, return the original string if path not found or value
// is null. A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples:
//   input: { a : { b : 1 }}
//   convert_uint64_value_to_string("/a/b", json, false)
//
//   input: { a : [ 1 ]}
//   convert_uint64_value_to_string("/a/0", json, false)
//
//   input: { "a~b" : [ 1 ]}
//   convert_uint64_value_to_string("/a~0b/0", json, false)
//
//   input: { "a/b" : [ 1 ]}
//   convert_uint64_value_to_string("/a~1b/0", json, false)
//
//   input: { a : null }
//   convert_uint64_value_to_string("/a", json, true)
pub fn convert_uint64_value_to_string(path: &str, json: &str, optional: bool) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();
    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        if optional {
            return json.to_string();
        }
        return String::new();
    }
    let value = mutable_pointer.unwrap();
    if value.is_null() && optional {
        return json.to_string();
    }
    if !value.is_u64() {
        return String::new();
    }
    *value = serde_json::Value::String(value.to_string());
    serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
}

// Parses and re-serializes json with the value at path converted from a int64
// to a string representation of the same number.
// Returns an empty String if such conversion is not possible.
// When optional is true, return the original string if path not found or value
// is null. A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples:
//   json: { a : { b : 1 }}
//   convert_int64_value_to_string("/a/b", json, false)
//
//   json: { a : [ 1 ]}
//   convert_int64_value_to_string("/a/0", json, false)
//
//   json: { "a~b" : [ 1 ]}
//   convert_int64_value_to_string("/a~0b/0", json, false)
//
//   json: { "a/b" : [ 1 ]}
//   convert_int64_value_to_string("/a~1b/0", json, false)
//
//   json: { a : null }
//   convert_int64_value_to_string("/a", json, true)
pub fn convert_int64_value_to_string(path: &str, json: &str, optional: bool) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();

    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        if optional {
            return json.to_string();
        }
        return String::new();
    }
    let value = mutable_pointer.unwrap();
    if value.is_null() && optional {
        return json.to_string();
    }
    if !value.is_i64() {
        return String::new();
    }
    *value = serde_json::Value::String(value.to_string());
    serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
}

// Parses and re-serializes json with the value at path converted from a string
// to a uint64 representation of the same number.
// Returns an empty String if such conversion is not possible.
// When optional is true, return the original string if path not found or value
// is null. A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples:
//   json: { a : { b : '1' }} -> { a : { b : 1 }}
//   convert_string_value_to_uint64("/a/b", json, false)
//
//   json: { a : [ '1' ]} -> { a : [ 1 ]}
//   convert_string_value_to_uint64("/a/0", json, false)
//
//   json: { "a~c" : { b : '1' }} -> { "a~c" : { b : 1 }}
//   convert_string_value_to_uint64("/a~0c/b", json, false)
//
//   json: { "a/b" : [ '1' ]} -> { "a/b" : [ 1 ]}
//   convert_string_value_to_uint64("/a~1b/0", json, false)
//
//   json: { a : null }
//   convert_string_value_to_uint64("/a", json, true)
pub fn convert_string_value_to_uint64(path: &str, json: &str, optional: bool) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();
    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        if optional {
            return json.to_string();
        }
        return String::new();
    }
    let value = mutable_pointer.unwrap();
    if value.is_null() && optional {
        return json.to_string();
    }
    if !value.is_string() {
        return String::new();
    }
    let unwrapped_number = value.as_str().unwrap();
    let uint64 = unwrapped_number.parse::<u64>();
    if let Ok(uint64) = uint64 {
        *value = serde_json::Value::from(uint64);
        return serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into());
    }
    String::new()
}

// Parses and re-serializes json with the value at path converted from a string
// to a int64 representation of the same number.
// Returns an empty String if such conversion is not possible.
// When optional is true, return the original string if path not found or value
// is null. A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples:
//   json: { a : { b : '1' }} -> { a : { b : 1 }}
//   convert_string_value_to_int64("/a/b", json, false)
//
//   json: { a : [ '1' ]} -> { a : [ 1 ]}
//   convert_string_value_to_int64("/a/0", json, false)
//
//   json: { "a~c" : { b : '1' }} -> { "a~c" : { b : 1 }}
//   convert_string_value_to_uint64("/a~0c/b", json, false)
//
//   json: { "a/b" : [ '1' ]} -> { "a/b" : [ 1 ]}
//   convert_string_value_to_uint64("/a~1b/0", json, false)
//
//   json: { a : null }
//   convert_string_value_to_uint64("/a", json, true)
pub fn convert_string_value_to_int64(path: &str, json: &str, optional: bool) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();
    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        if optional {
            return json.to_string();
        }
        return String::new();
    }
    let value = mutable_pointer.unwrap();
    if value.is_null() && optional {
        return json.to_string();
    }
    if !value.is_string() {
        return String::new();
    }
    let unwrapped_number = value.as_str().unwrap();
    let int64 = unwrapped_number.parse::<i64>();
    if let Ok(int64) = int64 {
        *value = serde_json::Value::from(int64);
        return serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into());
    }

    String::new()
}

// Parses and re-serializes json with uint64 values for the given key of the
// object located at path_to_object for all objects in the array located at
// path_to_array. Original json string will be returned if object array at path
// is not found. When the target uint64 value at key is not found in an object
// or is null, the value will be unchanged.
// Examples:
//   in: {a: [{"key": 1}, {"key": 2}, {"key": null]}
//   convert_uint64_in_object_array_to_string("/a", "", "key", json)
//   out: {a: [{"key": "1"}, {"key": "2"}, {"key": null}]}
//
//   in: {a: [{ "b": {"key": 1}}, {"b": {"key": 2}}, {"b": {"key": null]}}
//   convert_uint64_in_object_array_to_string("/a", "/b", "key", json)
//   out: {a: [{"b": {"key": "1"}}, {"b": {"key": "2"}}, {"b": {"key": null}}]}
pub fn convert_uint64_in_object_array_to_string(
    path_to_array: &str,
    path_to_object: &str,
    key: &str,
    json: &str,
) -> String {
    let mut unwrapped_value: serde_json::Value = match serde_json::from_str(&json) {
        Ok(value) => value,
        Err(_) => return String::new(),
    };

    let mutable_pointer = match unwrapped_value.pointer_mut(path_to_array) {
        Some(objects) => objects,
        None => return json.to_string(), // path_to_array not found
    };

    if let Some(objects) = mutable_pointer.as_array_mut() {
        for object in objects {
            if object.is_null() {
                continue;
            }

            let value = match object.pointer_mut(path_to_object) {
                Some(mutable_pointer2) => match mutable_pointer2.as_object_mut() {
                    Some(mutable_object2) => match mutable_object2.get_mut(key) {
                        Some(value) => value,
                        None => continue, // key not found
                    },
                    None => continue, // path_to_object not found
                },
                None => continue, // path_to_object not found
            };

            if value.is_null() {
                continue;
            }
            if !value.is_u64() {
                return String::new();
            }
            *value = serde_json::Value::String(value.to_string());
        }
    } else {
        return String::new();
    }

    serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
}

/// Parses and re-serializes json with all numbers (`u64`/`i64`/`f64`)
/// converted to strings, applied recursively at the specified path.
///
/// Values other than `u64`/`i64`/`f64` are unchanged. The fields could be
/// arbitrarily nested, as the conversion is applied to the entire JSON
/// recursively. Returns an empty String if such conversion is not possible.
///
/// Known double fields in the JSON may have `0` as the value, which will be
/// interpreted as a `u64`. For this reason, `f64` values are also converted to
/// string to be consistent with how we handle `u64`. Callers must extract the
/// appropriate values from the string fields using the following Chromium
/// helpers:
///   * `u64` -> `base::StringToUint64()`
///   * `i64` -> `base::StringToInt64()`
///   * `f64` -> `base::StringToDouble()`
///
/// # Arguments
/// * `json` - A arbitrary JSON string
/// * `path` - A JSON pointer path to the field where the conversion is applied
///   recursively. An empty string indicates the root of the JSON.
///
/// # Examples
///
/// ```js
/// {"a":1,"b":-1,"c":3.14} -> {"a":"1","b":"-1","c":"3.14"}
/// {"some":[{"deeply":{"nested":[{"key":123}]}}]} ->
///     {"some":[{"deeply":{"nested":[{"key":"123"}]}}]}
/// json={"a":1,"outer":{"inner":2}}, path="/outer" ->
///     {"a":1,"outer":{"inner":"2"}}
/// ```
pub fn convert_all_numbers_to_string(json: &str, path: &str) -> String {
    use serde_json::Value;

    fn convert_recursively(json: &mut Value) {
        match json {
            Value::Number(n) if n.is_u64() || n.is_i64() || n.is_f64() => {
                *json = Value::String(n.to_string());
            }
            Value::Array(a) => a.iter_mut().for_each(convert_recursively),
            Value::Object(o) => o.values_mut().for_each(convert_recursively),
            _ => (),
        }
    }

    serde_json::from_str::<Value>(json)
        .map(|mut v| {
            v.pointer_mut(path).map(convert_recursively);
            v.to_string()
        })
        .unwrap_or_else(|_| "".into())
}

/// Parses and re-serializes json with all numbers (`u64`/`i64`/`f64`),
/// converted to strings and empty arrays [], `null` values in array
/// or `null` valued properties removed, applied recursively
/// at the specified path.
///
/// Non `null` and other than `u64`/`i64`/`f64` values are unchanged.
/// The fields could be arbitrarily nested, as the conversion is applied
/// to the entire JSON recursively. Returns an empty String if such
/// conversion is not possible.
///
/// # Arguments
/// * `json` - A arbitrary JSON string
/// * `path` - A JSON pointer path to the field where the conversion is applied
///   recursively. An empty string indicates the root of the JSON.
///
/// # Examples
///
/// ```js
/// json={"a":1,"b":null,"c":"string","d":[],"e":[1,null],"f":[null]}, path=""
///  -> {"a":"1","c":"string","e":["1"]}
///
/// json={"a":1,"b":[{"bai1":1},{"bai2":null},{"bai3":"3"}],"c":"string",
/// "d": null}, path="/b" -> {"a":"1","b":[{"bai1":"1"},{"bai3":"3"}],
/// "c":"string","d":null}
/// ```
pub fn convert_all_numbers_to_string_and_remove_null_values(json: &str,
    path: &str) -> String {
    use serde_json::Value;
    fn convert_recursively(value: &mut Value) -> bool {
        let mut result = true;
        match value {
            Value::Number(n) if n.is_u64() || n.is_i64() || n.is_f64() => {
                *value = Value::String(n.to_string());
            }
            Value::Object(map) => {
                map.retain(|_, val| {
                    if !val.is_null() {
                        return convert_recursively(val);
                    }
                    return false;
                });
            },
            Value::Array(vec) => {
                for vec_item in vec.iter_mut() {
                    convert_recursively(vec_item);
                }
                vec.retain(|vec_item| {
                    if let Some(object) = vec_item.as_object() {
                        !object.is_empty()
                    } else {
                        !vec_item.is_null()
                    }
                });
                if vec.is_empty() {
                    result = false;
                }
            },
            Value::String(s) => { result = !s.is_empty(); }
            Value::Null => { result = false; }
            _ => ()
        };

        result
    }

    serde_json::from_str::<Value>(json)
        .map(|mut v| {
            v.pointer_mut(path).map(convert_recursively);
            v.to_string()
        })
        .unwrap_or_else(|_| "".into())
}
