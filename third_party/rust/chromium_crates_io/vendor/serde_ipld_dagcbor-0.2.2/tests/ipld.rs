use std::collections::BTreeMap;

use libipld_core::ipld::Ipld;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
struct TupleStruct(String, i32, u64);

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
struct UnitStruct;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
struct Struct<'a> {
    tuple_struct: TupleStruct,
    tuple: (String, f32, f64),
    map: BTreeMap<String, String>,
    bytes: &'a [u8],
    array: Vec<String>,
}

use std::iter::FromIterator;

#[allow(clippy::useless_format)]
#[test]
fn serde() {
    let tuple_struct = TupleStruct(format!("test"), -60, 3000);

    let tuple = (format!("hello"), -50.004097, -12.094635556478);

    let map = BTreeMap::from_iter(
        [
            (format!("key1"), format!("value1")),
            (format!("key2"), format!("value2")),
            (format!("key3"), format!("value3")),
            (format!("key4"), format!("value4")),
        ]
        .iter()
        .cloned(),
    );

    let bytes = b"test byte string";

    let array = vec![format!("one"), format!("two"), format!("three")];

    let data = Struct {
        tuple_struct,
        tuple,
        map,
        bytes,
        array,
    };

    let ipld = libipld_core::serde::to_ipld(data.clone()).unwrap();
    println!("{:?}", ipld);

    let data_ser = serde_ipld_dagcbor::to_vec(&ipld).unwrap();
    let data_de_ipld: Ipld = serde_ipld_dagcbor::from_slice(&data_ser).unwrap();

    fn as_object(ipld: &Ipld) -> &BTreeMap<String, Ipld> {
        if let Ipld::Map(ref v) = ipld {
            return v;
        }
        panic!()
    }

    for ((k1, v1), (k2, v2)) in as_object(&ipld).iter().zip(as_object(&data_de_ipld).iter()) {
        assert_eq!(k1, k2);
        assert_eq!(v1, v2);
    }

    assert_eq!(ipld, data_de_ipld);
}

#[test]
fn unit_struct_not_supported() {
    let unit_array = vec![UnitStruct, UnitStruct, UnitStruct];
    let ipld = libipld_core::serde::to_ipld(unit_array);
    assert!(ipld.is_err());
}

#[derive(Debug, Deserialize, Serialize)]
struct SmallStruct {
    spam: u32,
    eggs: u32,
}
