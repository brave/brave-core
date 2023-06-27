use crate::strobe::{SecParam, Strobe};

use std::{boxed::Box, fs::File, path::Path, string::String, vec::Vec};

use hex;
use serde::{de::Error as SError, Deserialize, Deserializer};
use serde_json;

// This is the top-level structure of the JSON we find in the test vectors
#[derive(Deserialize)]
struct TestHead {
    proto_string: String,
    #[serde(deserialize_with = "sec_param_from_bits")]
    security: SecParam,
    operations: Vec<TestOp>,
}

// Each individual test case looks like this
#[derive(Deserialize)]
struct TestOp {
    name: String,
    meta: bool,
    #[serde(deserialize_with = "bytes_from_hex")]
    input_data: Vec<u8>,
    stream: bool,
    #[serde(default, rename = "output", deserialize_with = "bytes_from_hex_opt")]
    expected_output: Option<Vec<u8>>,
    #[serde(default, rename = "state_after", deserialize_with = "bytes_from_hex")]
    expected_state_after: Vec<u8>,
}

// Tells serde how to deserialize a `SecParam`
fn sec_param_from_bits<'de, D: Deserializer<'de>>(deserializer: D) -> Result<SecParam, D::Error>
where
    D: Deserializer<'de>,
{
    let b = u64::deserialize(deserializer)?;
    match b {
        128 => Ok(SecParam::B128),
        256 => Ok(SecParam::B256),
        n => Err(SError::custom(format!("Invalid security parameter: {}", n))),
    }
}

// Tells serde how to deserialize bytes from hex
fn bytes_from_hex<'de, D>(deserializer: D) -> Result<Vec<u8>, D::Error>
where
    D: Deserializer<'de>,
{
    let mut hex_str = String::deserialize(deserializer)?;
    // Prepend a 0 if it's not even length
    if hex_str.len() % 2 == 1 {
        hex_str.insert(0, '0');
    }
    hex::decode(hex_str).map_err(|e| SError::custom(format!("{:?}", e)))
}

// This function is a formality. Some fields are not present, so they're wrapped in Option in the
// above structs. Hence, the deserialization function must return an Option. The `default` pragma
// on the members ensures, however, that the value is None when the field is missing.
fn bytes_from_hex_opt<'de, D>(deserializer: D) -> Result<Option<Vec<u8>>, D::Error>
where
    D: Deserializer<'de>,
{
    bytes_from_hex(deserializer).map(Some)
}

// Recall that `ratchet` can take a length argument, so this is the most general type that
// represents the input to a STROBE operation
enum DataOrLength<'a> {
    Data(&'a mut [u8]),
    Length(usize),
}

// Given the name of the operation and meta flag, returns a closure that performs this operation.
// The types are kind of a mess, because the input and output types of the closure have to fit all
// possible STROBE operations.
fn get_op(op_name: String, meta: bool) -> Box<dyn for<'a> Fn(&mut Strobe, DataOrLength<'a>, bool)> {
    let f = move |s: &mut Strobe, dol: DataOrLength, more: bool| {
        let data = match dol {
            DataOrLength::Length(len) => {
                if !meta {
                    assert_eq!(
                        op_name.as_str(),
                        "RATCHET",
                        "Got length input without RATCHET op"
                    );
                    s.ratchet(len, more);
                    return;
                } else {
                    assert_eq!(
                        op_name.as_str(),
                        "RATCHET",
                        "Got length input without RATCHET op"
                    );
                    s.meta_ratchet(len, more);
                    return;
                }
            }
            DataOrLength::Data(data) => data,
        };

        // Note: we don't expect recv_MAC to work on random inputs. We test recv_MAC's
        // correctness in strobe.rs
        if !meta {
            match op_name.as_str() {
                "AD" => s.ad(data, more),
                "KEY" => s.key(data, more),
                "PRF" => s.prf(data, more),
                "send_CLR" => s.send_clr(data, more),
                "recv_CLR" => s.recv_clr(data, more),
                "send_ENC" => s.send_enc(data, more),
                "recv_ENC" => s.recv_enc(data, more),
                "send_MAC" => s.send_mac(data, more),
                "recv_MAC" => s.recv_mac(data).unwrap_or(()),
                "RATCHET" => panic!("Got RATCHET op without length input"),
                _ => panic!("Unexpected op name: {}", op_name),
            }
        } else {
            match op_name.as_str() {
                "AD" => s.meta_ad(data, more),
                "KEY" => s.meta_key(data, more),
                "PRF" => s.meta_prf(data, more),
                "send_CLR" => s.meta_send_clr(data, more),
                "recv_CLR" => s.meta_recv_clr(data, more),
                "send_ENC" => s.meta_send_enc(data, more),
                "recv_ENC" => s.meta_recv_enc(data, more),
                "send_MAC" => s.meta_send_mac(data, more),
                "recv_MAC" => s.meta_recv_mac(data).unwrap_or(()),
                "RATCHET" => panic!("Got RATCHET op without length input"),
                _ => panic!("Unexpected op name: {}", op_name),
            }
        }
    };
    Box::new(f)
}

// If Strobe state serialization is defined, then this function does a
// JSON serialization/deserialization round trip on the input state. This is for testing
// correctness of our serde impl.
#[cfg(feature = "serialize_secret_state")]
fn serde_round_trip(s: Strobe) -> Strobe {
    let b = serde_json::to_vec(&s).unwrap();
    let s: Strobe = serde_json::from_slice(&b).unwrap();
    s
}
// If the this feature isn't present, then this is the identity function
#[cfg(not(feature = "serialize_secret_state"))]
fn serde_round_trip(s: Strobe) -> Strobe {
    s
}

// Runs the test vector and compares to the expected output at each step of the way
fn test_against_vector<P: AsRef<Path>>(filename: P) {
    let file = File::open(filename).unwrap();
    let TestHead {
        proto_string,
        security,
        operations,
    } = serde_json::from_reader(file).unwrap();
    let mut s = Strobe::new(proto_string.as_bytes(), security);

    for test_op in operations.into_iter() {
        // Test the serde functionality while we're at it. Do a JSON serialization/deserialization
        // round trip on the state. If not defined, this is the identity function.
        s = serde_round_trip(s);

        // Destructure the operation
        let TestOp {
            name,
            meta,
            mut input_data,
            stream,
            expected_output,
            expected_state_after,
        } = test_op;

        if name != "init" {
            // RATCHET inputs are given as strings of zeros instead of lengths. So just take the
            // length of the string of zeros.
            let input = if &name == "RATCHET" {
                DataOrLength::Length(input_data.len())
            } else {
                DataOrLength::Data(input_data.as_mut_slice())
            };

            let op = get_op(name.clone(), meta);
            op(&mut s, input, stream);

            assert_eq!(&s.st.0[..], expected_state_after.as_slice());

            // Only test expected output if the test vector has output to test against
            if let Some(eo) = expected_output {
                // The input was presumably mutated;
                let output = input_data.as_slice();
                assert_eq!(output, eo.as_slice());
            }
        }
    }
}

#[test]
fn simple_test() {
    test_against_vector("kat/simple_test_vector.json");
}

#[test]
fn meta_test() {
    test_against_vector("kat/meta_test_vector.json");
}

#[test]
fn streaming_test() {
    test_against_vector("kat/streaming_test_vector.json");
}

#[test]
fn boundary_test() {
    test_against_vector("kat/boundary_test_vector.json");
}
