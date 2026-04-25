use std::collections::HashMap;

#[derive(Debug, PartialEq, Clone)]
pub enum JsonValue {
    Null,
    Boolean(bool),
    Str(String),
    Num(f64),
    Array(Vec<JsonValue>),
    Object(HashMap<String, JsonValue>),
}
