// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Solana from '@solana/web3.js'

// utils
import { getLocale } from '$web-common/locale'
import { SOLANA_SYSTEM_INSTRUCTION_DECODERS } from '../common/constants/solana'

// types
import { BraveWallet } from '../constants/types'

export type TypedSolanaInstructionWithParams = {
  instruction: Solana.TransactionInstruction
} & (
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
)

type SolanaInstructionParams = Solana.AdvanceNonceParams
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

export type SolanaInstructionParamKeys = keyof SolanaInstructionParams

export const getSolanaSystemInstructionParamsAndType = ({
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

  const instructionType = Solana.SystemInstruction.decodeInstructionType(instruction)

  const params = Solana.SystemInstruction[
    SOLANA_SYSTEM_INSTRUCTION_DECODERS[instructionType]
  ](instruction) || {}

  return {
    instruction,
    params,
    type: instructionType
  } as TypedSolanaInstructionWithParams
}

export const getTypedSolanaTxInstructions = (solTxData: BraveWallet.SolanaTxData): TypedSolanaInstructionWithParams[] => {
  const instructions: TypedSolanaInstructionWithParams[] = (solTxData?.instructions || []).map((instruction) => {
    return getSolanaSystemInstructionParamsAndType(instruction)
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
