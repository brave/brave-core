//! An example that serializes CpuId state, deserializes it back and passes it to a
//! CpuId instance with a custom reader.
//!
//! Run using:
//! `cargo run --features serialize --features serde_json --example serialize_deserialize`

use raw_cpuid::{CpuId, CpuIdReader, CpuIdResult};
use std::collections::HashMap;

#[derive(Clone)]
struct HashMapCpuIdReader {
    ht: HashMap<u64, CpuIdResult>,
}

impl CpuIdReader for HashMapCpuIdReader {
    fn cpuid2(&self, eax: u32, ecx: u32) -> CpuIdResult {
        let key = (eax as u64) << u32::BITS | ecx as u64;
        self.ht
            .get(&key)
            .unwrap_or(&CpuIdResult {
                eax: 0,
                ebx: 0,
                ecx: 0,
                edx: 0,
            })
            .clone()
    }
}

fn main() {
    let mut ht = HashMap::new();
    ht.insert(
        0x00000000_00000000u64,
        CpuIdResult {
            eax: 0x00000020,
            ebx: 0x756e6547,
            ecx: 0x6c65746e,
            edx: 0x49656e69,
        },
    );

    // Serialize to JSON and back
    let cpuid_as_json = serde_json::to_string(&ht).unwrap();
    let deserialized_ht: HashMap<u64, CpuIdResult> = serde_json::from_str(&cpuid_as_json).unwrap();

    // Create a CpuId instance with a custom reader
    let cpuid = CpuId::with_cpuid_reader(HashMapCpuIdReader {
        ht: deserialized_ht,
    });
    assert!(cpuid.get_vendor_info().unwrap().as_str() == "GenuineIntel");
}
