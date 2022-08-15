// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../constants/types'

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
} as const // Record<Solana.SystemInstructionType, keyof typeof Solana.SystemInstruction>

export const SOLANA_VOTE_PROGRAM_INSTRUCTION_DECODERS = {
  'Authorize': 'decodeAuthorize',
  'InitializeAccount': 'decodeInitializeAccount',
  'Withdraw': 'decodeWithdraw'
} as const // Record<Solana.VoteInstructionType, keyof typeof Solana.VoteInstruction>

export const SOLANA_STAKE_PROGRAM_INSTRUCTION_DECODERS = {
  'Authorize': 'decodeAuthorize',
  'AuthorizeWithSeed': 'decodeAuthorizeWithSeed',
  'Deactivate': 'decodeDeactivate',
  'Delegate': 'decodeDelegate',
  'Initialize': 'decodeInitialize',
  'Merge': 'decodeMerge',
  'Split': 'decodeSplit',
  'Withdraw': 'decodeWithdraw'
} as const // Record<Solana.StakeInstructionType, keyof typeof Solana.StakeInstruction>

export const SolanaTransactionTypes = [
  BraveWallet.TransactionType.SolanaSystemTransfer,
  BraveWallet.TransactionType.SolanaSPLTokenTransfer,
  BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
  BraveWallet.TransactionType.SolanaDappSignTransaction,
  BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
  BraveWallet.TransactionType.SolanaSwap
]
