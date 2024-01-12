use std::collections::BTreeMap;

use libipld_cbor::DagCborCodec;
use libipld_core::{codec::Codec, raw_value::RawValue};
use quickcheck::{empty_shrinker, Arbitrary, Gen, QuickCheck};

#[test]
fn raw_serde_roundtrip_test() {
    QuickCheck::new().quickcheck(raw_serde_roundtrip as fn(ValueArb) -> bool)
}

fn raw_serde_roundtrip(input: ValueArb) -> bool {
    let bytes = serde_cbor::to_vec(&input.0).unwrap();
    let value: RawValue<DagCborCodec> = DagCborCodec.decode(&bytes).unwrap();
    let bytes2 = DagCborCodec.encode(&value).unwrap();

    let output: serde_cbor::Value = serde_cbor::from_slice(&bytes2[..]).unwrap();
    input.0 == output
}

#[derive(Clone, Debug)]
struct ValueArb(pub serde_cbor::Value);
impl Arbitrary for ValueArb {
    fn arbitrary(g: &mut Gen) -> Self {
        Self(gen_value(g, 2))
    }
    fn shrink(&self) -> Box<dyn Iterator<Item = Self>> {
        match &self.0 {
            Value::Null => empty_shrinker(),
            Value::Bool(v) => Box::new(Arbitrary::shrink(v).map(Value::Bool).map(Self)),
            Value::Integer(v) => Box::new(
                Arbitrary::shrink(&(*v as i64))
                    .map(|i| Value::Integer(i as i128))
                    .map(Self),
            ),
            Value::Float(f) => Box::new(Arbitrary::shrink(f).map(Value::Float).map(Self)),
            Value::Bytes(v) => Box::new(Arbitrary::shrink(v).map(Value::Bytes).map(Self)),
            Value::Text(v) => Box::new(Arbitrary::shrink(v).map(Value::Text).map(Self)),
            Value::Array(a) => Box::new(
                a.clone()
                    .into_iter()
                    .map(|x| {
                        let slf = Self(x);
                        let shrunk = slf.shrink().map(|x| x.0).collect::<Vec<_>>();
                        shrunk
                    })
                    .map(Value::Array)
                    .map(Self),
            ),
            Value::Map(m) => Box::new(
                m.clone()
                    .into_iter()
                    .map(|(v0, v1)| {
                        let iter2 = Self(v1.clone());
                        Self(v0.clone())
                            .shrink()
                            .map(move |n| (n.0, v1.clone()))
                            .chain(iter2.shrink().map(move |n| (v0.clone(), n.0)))
                            .collect()
                    })
                    .map(Value::Map)
                    .map(Self),
            ),

            Value::Tag(t, v) => {
                let v = v.clone();
                let vc = v.clone();
                let t = *t;
                Box::new(
                    t.shrink()
                        .map(move |n| (n, vc.clone()))
                        .chain(Self(*v).shrink().map(move |n| (t, Box::new(n.0))))
                        .map(|(t, b)| Value::Tag(t, b))
                        .map(Self),
                )
            }
            _ => unreachable!(),
        }
    }
}

use serde_cbor::Value;
fn gen_value(g: &mut Gen, depth: u16) -> Value {
    let upper = if depth > 0 { 8 } else { 5 };
    match gen_range(g, 0, upper) {
        0 => Value::Null,
        1 => Value::Bool(Arbitrary::arbitrary(g)),
        // Range 2^64-1 to -2^64
        2 => {
            let sgn = bool::arbitrary(g);
            let v = u64::arbitrary(g) as i128;
            Value::Integer(if sgn { v } else { -v })
        }
        3 => Value::Float(f64::arbitrary(g)),
        4 => Value::Bytes(Arbitrary::arbitrary(g)),
        5 => Value::Text(Arbitrary::arbitrary(g)),
        // recursive variants
        6 => Value::Array(gen_vec(g, depth - 1)),
        7 => Value::Map(gen_map(g, depth - 1)),
        8 => Value::Tag(u64::arbitrary(g), Box::new(gen_value(g, depth - 1))),
        _ => unreachable!(),
    }
}

fn gen_range(g: &mut Gen, lower: usize, upper: usize) -> usize {
    assert!(lower < upper);
    let range = (0..=upper).collect::<Vec<_>>();
    *g.choose(&range[..]).unwrap()
}

fn gen_vec(g: &mut Gen, depth: u16) -> Vec<Value> {
    let size = gen_range(g, 0, g.size());
    (0..size).map(|_| gen_value(g, depth)).collect()
}

fn gen_map(g: &mut Gen, depth: u16) -> BTreeMap<Value, Value> {
    let size = gen_range(g, 0, g.size());
    (0..size)
        .map(|_| (gen_value(g, depth), gen_value(g, depth)))
        .collect()
}
