#![cfg(feature = "const-generics")]

use serde_derive::{Serialize, Deserialize};
use serde_big_array::BigArray;

#[derive(Serialize, Deserialize)]
struct S {
    #[serde(with = "BigArray")]
    arr: [u8; 64],
    #[serde(with = "BigArray")]
    arr2: [u8; 65],
}

#[test]
fn test() {
    let s = S { arr: [1; 64], arr2: [1; 65] };
    let j = serde_json::to_string(&s).unwrap();
    let s_back = serde_json::from_str::<S>(&j).unwrap();
    assert!(&s.arr[..] == &s_back.arr[..]);
}
