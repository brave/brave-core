#![no_std]

use serde_derive::{Serialize, Deserialize};
use serde_big_array::big_array;

const NUMBER: usize = 137;

big_array! {
    BigArray;
    NUMBER * NUMBER + 17, NUMBER, 42
}

#[derive(Serialize, Deserialize)]
struct S {
    #[serde(with = "BigArray")]
    arr_1: [u8; NUMBER * NUMBER + 17],
    #[serde(with = "BigArray")]
    arr_2: [u8; NUMBER],
    #[serde(with = "BigArray")]
    arr_3: [u8; 42],
}

#[test]
fn test() {
    let s = S {
        arr_1: [1; NUMBER * NUMBER + 17],
        arr_2: [2; NUMBER],
        arr_3: [3; 42],
    };
    let j = serde_json::to_string(&s).unwrap();
    let s_back = serde_json::from_str::<S>(&j).unwrap();
    assert!(&s.arr_1[..] == &s_back.arr_1[..]);
    assert!(&s.arr_2[..] == &s_back.arr_2[..]);
    assert!(&s.arr_3[..] == &s_back.arr_3[..]);
}
