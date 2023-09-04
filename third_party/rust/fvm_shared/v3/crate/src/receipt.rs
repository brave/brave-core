// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::Cid;
use fvm_ipld_encoding::tuple::{Deserialize_tuple, Serialize_tuple};
use fvm_ipld_encoding::RawBytes;

use crate::error::ExitCode;

/// Result of a state transition from a message
#[derive(Serialize_tuple, Deserialize_tuple, Debug, PartialEq, Eq, Clone)]
pub struct Receipt {
    pub exit_code: ExitCode,
    pub return_data: RawBytes,
    pub gas_used: u64,
    /// If any actor events were emitted during execution, this field will contain the CID of the
    /// root of the AMT holding the StampedEvents. Otherwise, this will be None (serializing to a
    /// CBOR NULL value on the wire).
    pub events_root: Option<Cid>, // Amt<Event>
}
