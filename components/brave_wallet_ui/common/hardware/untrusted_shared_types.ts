// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types shared between wallet page and ledger/trezor untrusted iframes.
// Should not depend on BraveWallet mojo
export const BridgeTypes = {
  EthLedger: 'EthLedger',
  SolLedger: 'SolLedger',
  FilLedger: 'FilLedger',
  BtcLedger: 'BtcLedger',
  EthTrezor: 'EthTrezor'
} as const

export type BridgeType = (typeof BridgeTypes)[keyof typeof BridgeTypes]
