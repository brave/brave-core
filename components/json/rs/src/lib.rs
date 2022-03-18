#[cxx::bridge(namespace = json)]
mod ffi {
    extern "Rust" {
      fn convert_uint64_to_unicode_string(path: &str, json: &str) -> String;
      fn convert_int64_to_unicode_string(path: &str, json: &str) -> String;
      fn convert_unicode_string_to_uint64(path: &str, json: &str) -> String;
      fn convert_unicode_string_to_int64(path: &str, json: &str) -> String;
    }
}

// Converts uint64 value to string and serializes json with new values.
// A path is a Unicode string with the reference tokens separated by /. Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// Examples: convert_uint64_to_unicode_string("/a/b", json) for { a : { b : 1 }}
//           convert_uint64_to_unicode_string("/a/0", json) for { a : [ 1 ]}
pub fn convert_uint64_to_unicode_string(path: &str, json: &str) -> String {
  let mut unwrapped_value: serde_json::Value = serde_json::from_str(&json).unwrap_or_else(|e| {
    eprintln!("Failed to parse JSON: {}", e);
    return serde_json::Value::Null;
  });
  if unwrapped_value.is_null() {
    return String::new();
  }
  let mutable_pointer = unwrapped_value.pointer_mut(path);
  if mutable_pointer.is_none() {
    return String::new();
  }
  let value = mutable_pointer.unwrap();
  if value.is_null() {
    return String::new();
  }
  if !value.is_u64() {
    return String::new();
  }
  *value = serde_json::Value::String(value.to_string());
  return serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
}

// Converts int64 value to string and serializes json with new values.
// A path is a Unicode string with the reference tokens separated by /. Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// Examples: convert_int64_to_unicode_string("/a/b", json) for { a : { b : 1 }}
//           convert_int64_to_unicode_string("/a/0", json) for { a : [ 1 ]}
pub fn convert_int64_to_unicode_string(path: &str, json: &str) -> String {
  let mut unwrapped_value: serde_json::Value = serde_json::from_str(&json).unwrap_or_else(|e| {
    eprintln!("Failed to parse JSON: {}", e);
    return serde_json::Value::Null;
  });
  if unwrapped_value.is_null() {
    return String::new();
  }
  let mutable_pointer = unwrapped_value.pointer_mut(path);
  if mutable_pointer.is_none() {
    return String::new();
  }
  let value = mutable_pointer.unwrap();
  if value.is_null() {
    return String::new();
  }
  if !value.is_i64() {
    return String::new();
  }
  *value = serde_json::Value::String(value.to_string());
  return serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
}

// Converts string value to uint64 and serializes json with new values.
// A path is a Unicode string with the reference tokens separated by /. Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// Examples: convert_unicode_string_to_uint64("/a/b", json) for { a : { b : '1' }} -> { a : { b : 1 }}
//           convert_unicode_string_to_uint64("/a/0", json) for { a : [ '1' ]} -> { a : [ 1 ]}
pub fn convert_unicode_string_to_uint64(path: &str, json: &str) -> String {
  let mut unwrapped_value: serde_json::Value = serde_json::from_str(&json).unwrap_or_else(|e| {
    eprintln!("Failed to parse JSON: {}", e);
    return serde_json::Value::Null;
  });
  if unwrapped_value.is_null() {
    return String::new();
  }
  let mutable_pointer = unwrapped_value.pointer_mut(path);
  if mutable_pointer.is_none() {
    return String::new();
  }
  let value = mutable_pointer.unwrap();
  if value.is_null() {
    return String::new();
  }
  if !value.is_string() {
    return String::new();
  }
  let unwrapped_number = value.as_str().unwrap();
  let uint64 = unwrapped_number.parse::<u64>();
  if let Ok(uint64) = uint64 {
    *value = serde_json::Value::from(uint64);
    return serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
  }
  return String::new();
}

// Converts string value to int64 and serializes json with new values.
// A path is a Unicode string with the reference tokens separated by /. Inside tokens / is replaced by ~1 and ~ is replaced by ~0.
// Examples: convert_unicode_string_to_int64("/a/b", json) for { a : { b : '1' }} -> { a : { b : 1 }}
//           convert_unicode_string_to_int64("/a/0", json) for { a : [ '1' ]} -> { a : [ 1 ]}
pub fn convert_unicode_string_to_int64(path: &str, json: &str) -> String {
  let mut unwrapped_value: serde_json::Value = serde_json::from_str(&json).unwrap_or_else(|e| {
    eprintln!("Failed to parse JSON: {}", e);
    return serde_json::Value::Null;
  });
  if unwrapped_value.is_null() {
    return String::new();
  }
  let mutable_pointer = unwrapped_value.pointer_mut(path);
  if mutable_pointer.is_none() {
    return String::new();
  }
  let value = mutable_pointer.unwrap();
  if value.is_null() {
    return String::new();
  }
  if !value.is_string() {
    return String::new();
  }
  let unwrapped_number = value.as_str().unwrap();
  let int64 = unwrapped_number.parse::<i64>();
  if let Ok(int64) = int64 {
    *value = serde_json::Value::from(int64);
    return serde_json::to_string(&unwrapped_value).unwrap_or_else(|_| "".into())
  }
  
  return String::new();
}
