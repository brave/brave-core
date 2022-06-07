// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const SOLANA_SYSTEM_INSTRUCTION_DECODERS = {
  'AdvanceNonceAccount': 'decodeNonceAdvance',
  'Allocate': 'decodeAllocate',
  'AllocateWithSeed': 'decodeAllocateWithSeed',
  'Assign': 'decodeAssign',
  'AssignWithSeed': 'decodeAssignWithSeed',
  'AuthorizeNonceAccount': 'decodeNonceAuthorize',
  'Create': 'decodeCreateAccount',
  'CreateWithSeed': 'decodeCreateWithSeed',
  'InitializeNonceAccount': 'decodeNonceInitialize',
  'Transfer': 'decodeTransfer',
  'TransferWithSeed': 'decodeTransferWithSeed',
  'WithdrawNonceAccount': 'decodeNonceWithdraw'
} as const
