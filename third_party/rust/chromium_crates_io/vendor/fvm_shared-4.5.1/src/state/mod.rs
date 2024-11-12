// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use cid::Cid;
use fvm_ipld_encoding::repr::*;
use fvm_ipld_encoding::tuple::*;
use num_traits::Zero;
#[cfg(feature = "arb")]
use quickcheck::Arbitrary;
use serde::{Deserialize, Serialize};

use crate::address::Address;
use crate::econ::TokenAmount;
use crate::EMPTY_ARR_CID;

/// Specifies the version of the state tree
#[derive(Debug, PartialEq, Eq, Clone, Copy, PartialOrd, Serialize_repr, Deserialize_repr)]
#[repr(u64)]
pub enum StateTreeVersion {
    /// Corresponds to actors < v2
    V0,
    /// Corresponds to actors = v2
    V1,
    /// Corresponds to actors = v3
    V2,
    /// Corresponds to actors = v4
    V3,
    /// Corresponds to actors >= v5
    V4,
    /// Corresponding to actors >= v10
    V5,
}

/// State root information. Contains information about the version of the state tree,
/// the root of the tree, and a link to the information about the tree.
#[derive(Deserialize_tuple, Serialize_tuple)]
pub struct StateRoot {
    /// State tree version
    pub version: StateTreeVersion,

    /// Actors tree. The structure depends on the state root version.
    pub actors: Cid,

    /// Info. The structure depends on the state root version.
    pub info: Cid,
}

/// Empty state tree information. This is serialized as an array for future proofing.
#[derive(Default, Deserialize, Serialize)]
#[serde(transparent)]
pub struct StateInfo0([(); 0]);

/// State of all actor implementations.
#[derive(PartialEq, Eq, Clone, Debug, Serialize_tuple, Deserialize_tuple)]
pub struct ActorState {
    /// Link to code for the actor.
    pub code: Cid,
    /// Link to the state of the actor.
    pub state: Cid,
    /// Sequence of the actor.
    pub sequence: u64,
    /// Tokens available to the actor.
    pub balance: TokenAmount,
    /// The actor's "delegated" address, if assigned.
    ///
    /// This field is set on actor creation and never modified.
    pub delegated_address: Option<Address>,
}

/// Error returned when attempting to deduct funds with an insufficient balance.
#[derive(thiserror::Error, Debug)]
pub enum InvalidTransfer {
    #[error("insufficient funds when deducting funds ({amount}) from balance ({balance})")]
    InsufficientBalance {
        amount: TokenAmount,
        balance: TokenAmount,
    },
    #[error("cannot deposite/deduct a negative amount of funds ({0})")]
    NegativeAmount(TokenAmount),
}

impl ActorState {
    /// Constructor for actor state
    pub fn new(
        code: Cid,
        state: Cid,
        balance: TokenAmount,
        sequence: u64,
        address: Option<Address>,
    ) -> Self {
        Self {
            code,
            state,
            sequence,
            balance,
            delegated_address: address,
        }
    }

    /// Construct a new empty actor with the specified code.
    pub fn new_empty(code: Cid, delegated_address: Option<Address>) -> Self {
        ActorState {
            code,
            state: EMPTY_ARR_CID,
            sequence: 0,
            balance: TokenAmount::zero(),
            delegated_address,
        }
    }

    /// Safely deducts funds from an Actor
    pub fn deduct_funds(&mut self, amt: &TokenAmount) -> Result<(), InvalidTransfer> {
        if amt.is_negative() {
            return Err(InvalidTransfer::NegativeAmount(amt.clone()));
        }
        if &self.balance < amt {
            return Err(InvalidTransfer::InsufficientBalance {
                amount: amt.clone(),
                balance: self.balance.clone(),
            });
        }
        self.balance -= amt;

        Ok(())
    }
    /// Deposits funds to an Actor
    pub fn deposit_funds(&mut self, amt: &TokenAmount) -> Result<(), InvalidTransfer> {
        if amt.is_negative() {
            Err(InvalidTransfer::NegativeAmount(amt.clone()))
        } else {
            self.balance += amt;
            Ok(())
        }
    }
}

#[cfg(feature = "arb")]
impl Arbitrary for ActorState {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        Self {
            code: Cid::arbitrary(g),
            state: Cid::arbitrary(g),
            sequence: u64::arbitrary(g),
            balance: TokenAmount::arbitrary(g),
            delegated_address: Option::arbitrary(g),
        }
    }
}
