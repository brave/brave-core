// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use cid::Cid;
use fvm_ipld_encoding::tuple::*;

#[derive(Clone, Debug, Copy, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct UpgradeInfo {
    // the old code cid we are upgrading from
    pub old_code_cid: Cid,
}
