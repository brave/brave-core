// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as Solana from '@solana/web3.js'

// utils
import { getLocale } from '../../common/locale'
import {
  SOLANA_STAKE_PROGRAM_INSTRUCTION_DECODERS,
  SOLANA_SYSTEM_INSTRUCTION_DECODERS,
  SOLANA_VOTE_PROGRAM_INSTRUCTION_DECODERS
} from '../common/constants/solana'

// types
import { BraveWallet, SerializableSolanaTxData } from '../constants/types'

export type TypedSolanaInstructionWithParams = {
  instruction: Solana.TransactionInstruction
} & (
  // System Program
  | { params: Solana.AdvanceNonceParams, type: 'AdvanceNonceAccount' }
  | { params: Solana.AllocateParams, type: 'Allocate' }
  | { params: Solana.AllocateWithSeedParams, type: 'AllocateWithSeed' }
  | { params: Solana.AllocateWithSeedParams, type: 'AllocateWithSeed' }
  | { params: Solana.AssignParams, type: 'Assign' }
  | { params: Solana.AssignWithSeedParams, type: 'AssignWithSeed' }
  | { params: Solana.AuthorizeNonceParams, type: 'AuthorizeNonceAccount' }
  | { params: Solana.CreateAccountParams, type: 'Create' }
  | { params: Solana.CreateAccountWithSeedParams, type: 'CreateWithSeed' }
  | { params: Solana.InitializeNonceParams, type: 'InitializeNonceAccount' }
  | { params: Solana.TransferParams, type: 'Transfer' }
  | { params: Solana.TransferWithSeedParams, type: 'TransferWithSeed' }
  | { params: Solana.WithdrawNonceParams, type: 'WithdrawNonceAccount' }

  // Vote Program
  | { params: Solana.AuthorizeVoteParams, type: 'Authorize' }
  | { params: Solana.CreateVoteAccountParams, type: 'InitializeAccount' }
  | { params: Solana.WithdrawFromVoteAccountParams, type: 'Withdraw' }

  // Staking
  | { params: Solana.AuthorizeStakeParams, type: 'Authorize' }
  | { params: Solana.AuthorizeWithSeedStakeParams, type: 'AuthorizeWithSeed' }
  | { params: Solana.DeactivateStakeParams, type: 'Deactivate' }
  | { params: Solana.DelegateStakeParams, type: 'Delegate' }
  | { params: Solana.InitializeStakeParams, type: 'Initialize' }
  | { params: Solana.MergeStakeParams, type: 'Merge' }
  | { params: Solana.SplitStakeParams, type: 'Split' }
  | { params: Solana.WithdrawStakeParams, type: 'Withdraw' }

  // Unknown
  | { params: {}, type: 'Unknown' }
)

type SolanaInstructionParams =
  // System Program
  & Solana.AdvanceNonceParams
  & Solana.AllocateParams
  & Solana.AllocateWithSeedParams
  & Solana.AllocateWithSeedParams
  & Solana.AssignParams
  & Solana.AssignWithSeedParams
  & Solana.AuthorizeNonceParams
  & Solana.CreateAccountParams
  & Solana.CreateAccountWithSeedParams
  & Solana.InitializeNonceParams
  & Solana.TransferParams
  & Solana.TransferWithSeedParams
  & Solana.WithdrawNonceParams

  // Voting
  & Solana.AuthorizeVoteParams
  & Solana.CreateVoteAccountParams
  & Solana.WithdrawFromVoteAccountParams

  // Staking
  & Solana.MergeStakeParams
  & Solana.SplitStakeParams
  & Solana.DelegateStakeParams
  & Solana.WithdrawStakeParams
  & Solana.AuthorizeStakeParams
  & Solana.DeactivateStakeParams
  & Solana.InitializeStakeParams
  & Solana.CreateStakeAccountParams
  & Solana.SplitStakeWithSeedParams
  & Solana.AuthorizeWithSeedStakeParams
  & Solana.CreateStakeAccountWithSeedParams

export type SolanaInstructionParamKeys = keyof SolanaInstructionParams

export const getSolanaTransactionInstructionParamsAndType = ({
  accountMetas,
  data,
  programId
}: BraveWallet.SolanaInstruction): TypedSolanaInstructionWithParams => {
  const instruction: Solana.TransactionInstruction = new Solana.TransactionInstruction({
    data: Buffer.from(data),
    programId: new Solana.PublicKey(programId),
    keys: accountMetas.map((meta) => ({
      isSigner: meta.isSigner,
      isWritable: meta.isWritable,
      pubkey: new Solana.PublicKey(meta.pubkey)
    }))
  })

  let instructionType = 'Unknown'
  let params = {}

  if (instruction.programId.equals(Solana.SystemProgram.programId)) {
    instructionType = Solana.SystemInstruction.decodeInstructionType(instruction)
    params = Solana.SystemInstruction[
      SOLANA_SYSTEM_INSTRUCTION_DECODERS[instructionType]
    ](instruction) || {}
  }

  if (instruction.programId.equals(Solana.VoteProgram.programId)) {
    instructionType = Solana.VoteInstruction.decodeInstructionType(instruction)
    params = Solana.VoteInstruction[
      SOLANA_VOTE_PROGRAM_INSTRUCTION_DECODERS[instructionType]
    ](instruction) || {}
  }

  if (instruction.programId.equals(Solana.StakeProgram.programId)) {
    instructionType = Solana.StakeInstruction.decodeInstructionType(instruction)
    params = Solana.StakeInstruction[
      SOLANA_STAKE_PROGRAM_INSTRUCTION_DECODERS[instructionType]
    ](instruction)
  }

  return {
    instruction,
    params,
    type: instructionType
  } as TypedSolanaInstructionWithParams
}

export const getTypedSolanaTxInstructions = (solTxData: SerializableSolanaTxData): TypedSolanaInstructionWithParams[] => {
  const instructions: TypedSolanaInstructionWithParams[] = (solTxData?.instructions || []).map((instruction) => {
    return getSolanaTransactionInstructionParamsAndType(instruction)
  })
  return instructions || []
}

export const getSolanaInstructionParamKeyName = (key: SolanaInstructionParamKeys) => {
  return ({
    fromPubkey: getLocale('braveWalletSolanaParamKeyFromPubkey'),
    toPubkey: getLocale('braveWalletSolanaParamKeyToPubkey'),
    lamports: getLocale('braveWalletSolanaParamKeyLamports'),
    newAccountPubkey: getLocale('braveWalletSolanaParamKeyNewAccountPubkey')
  } as Partial<Record<SolanaInstructionParamKeys, string>>)[key] || key
}
