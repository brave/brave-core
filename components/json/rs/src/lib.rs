#[cxx::bridge(namespace = json)]
mod ffi {
    extern "Rust" {
        fn convert_uint64_value_to_string(path: &str, json: &str) -> String;
        fn convert_int64_value_to_string(path: &str, json: &str) -> String;
        fn convert_string_value_to_uint64(path: &str, json: &str) -> String;
        fn convert_string_value_to_int64(path: &str, json: &str) -> String;
    }
}

// Parses and re-serializes json with the value at path converted from a uint64
// to a string representation of the same number.
// Returns an empty String if such conversion is not possible.
// A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples: convert_uint64_value_to_string("/a/b", json) for { a : { b : 1 }}
//           convert_uint64_value_to_string("/a/0", json) for { a : [ 1 ]}
//           convert_uint64_value_to_string("/a~0b/0", json) for { "a~b" : [ 1 ]}
//           convert_uint64_value_to_string("/a~1b/0", json) for { "a/b" : [ 1 ]}
pub fn convert_uint64_value_to_string(path: &str, json: &str) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();
    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        return String::new();
    }
    let value = mutable_pointer.unwrap();
    if !value.is_u64() {
        return String::new();
    }
    *value = serde_json::Value::String(value.to_string());
    serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
}

// Parses and re-serializes json with the value at path converted from a int64
// to a string representation of the same number.
// Returns an empty String if such conversion is not possible.
// A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples: convert_int64_to_unicode_string("/a/b", json) for { a : { b : 1 }}
//           convert_int64_to_unicode_string("/a/0", json) for { a : [ 1 ]}
//           convert_int64_to_unicode_string("/a~0b/0", json) for { "a~b" : [ 1 ]}
//           convert_int64_to_unicode_string("/a~1b/0", json) for { "a/b" : [ 1 ]}
pub fn convert_int64_value_to_string(path: &str, json: &str) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();

    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        return String::new();
    }
    let value = mutable_pointer.unwrap();
    if !value.is_i64() {
        return String::new();
    }
    *value = serde_json::Value::String(value.to_string());
    serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
}

// Parses and re-serializes json with the value at path converted from a string
// to a uint64 representation of the same number.
// Returns an empty String if such conversion is not possible.
// A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples: convert_string_value_to_uint64("/a/b", json) for { a : { b : '1' }} -> { a : { b : 1 }}
//           convert_string_value_to_uint64("/a/0", json) for { a : [ '1' ]} -> { a : [ 1 ]}
//           convert_string_value_to_uint64("/a~0c/b", json) for { "a~c" : { b : '1' }} -> { "a~c" : { b : 1 }}
//           convert_string_value_to_uint64("/a~1b/0", json) for { "a/b" : [ '1' ]} -> { "a/b" : [ 1 ]}
pub fn convert_string_value_to_uint64(path: &str, json: &str) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();
    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        return String::new();
    }
    let value = mutable_pointer.unwrap();
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
// A path is a Unicode string with the reference tokens separated by /.
// Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// For more information read [RFC6901](https://tools.ietf.org/html/rfc6901).
// Examples: convert_string_value_to_int64("/a/b", json) for { a : { b : '1' }} -> { a : { b : 1 }}
//           convert_string_value_to_int64("/a/0", json) for { a : [ '1' ]} -> { a : [ 1 ]}
//           convert_string_value_to_uint64("/a~0c/b", json) for { "a~c" : { b : '1' }} -> { "a~c" : { b : 1 }}
//           convert_string_value_to_uint64("/a~1b/0", json) for { "a/b" : [ '1' ]} -> { "a/b" : [ 1 ]}
pub fn convert_string_value_to_int64(path: &str, json: &str) -> String {
    let result_value = serde_json::from_str(&json);
    if result_value.is_err() {
        return String::new();
    }
    let mut unwrapped_value: serde_json::Value = result_value.unwrap();
    let mutable_pointer = unwrapped_value.pointer_mut(path);
    if mutable_pointer.is_none() {
        return String::new();
    }
    let value = mutable_pointer.unwrap();
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
