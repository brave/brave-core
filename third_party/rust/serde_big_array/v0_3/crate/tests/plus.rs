#![no_std]

use serde_big_array::big_array;
use serde_derive::{Deserialize, Serialize};

big_array! { BigArray; +127, }
big_array! { BigArrayNoTrailing; +127, 129 }

#[derive(Serialize, Deserialize)]
struct S {
    #[serde(with = "BigArray")]
    arr: [u8; 64],
    #[serde(with = "BigArray")]
    arr_2: [u8; 127],
    #[serde(with = "BigArrayNoTrailing")]
    arr_3: [u8; 129],
}

#[test]
fn test() {
    let s = S {
        arr: [1; 64],
        arr_2: [1; 127],
        arr_3: [1; 129],
    };
    let j = serde_json::to_string(&s).unwrap();
    let s_back = serde_json::from_str::<S>(&j).unwrap();
    assert!(&s.arr[..] == &s_back.arr[..]);
    assert!(&s.arr_2[..] == &s_back.arr_2[..]);
    assert!(&s.arr_3[..] == &s_back.arr_3[..]);
}
