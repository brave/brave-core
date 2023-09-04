# serde_tuple

De/serialize structs with named fields as array of values.

See: https://github.com/dtolnay/request-for-implementation/issues/3

# Usage

```rust
use serde_tuple::*;

#[derive(Serialize_tuple, Deserialize_tuple)]
pub struct Foo<'a> {
    bar: &'a str,
    baz: i32
}

let foo = Foo { bar: "Yes", baz: 22 };
let json = serde_json::to_string(&foo).unwrap();
println!("{}", &json);
// # => ["Yes",22]
```

License: MIT