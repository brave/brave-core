// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  SerializableSolanaTxData
} from '../constants/types'

// utils
import { findAccountName } from './account-utils'
import { getGetCleanedMojoEnumKeys } from './enum-utils'
import { lamportsToSol } from './web3-utils'

export const SolanaSystemInstructionKeys = getGetCleanedMojoEnumKeys(
  BraveWallet.SolanaSystemInstruction
)

export const SolanaTokenInstructionKeys = getGetCleanedMojoEnumKeys(
  BraveWallet.SolanaTokenInstruction
)

export type SolanaSystemInstructionType =
  typeof SolanaSystemInstructionKeys[number]

export type SolanaTokenInstructionType =
  typeof SolanaTokenInstructionKeys[number]

export type TypedSolanaInstructionWithParams = {
  accountMetas: BraveWallet.SolanaAccountMeta[]
  programId: string
  accountParams: BraveWallet.SolanaInstructionParam[]
  type?: SolanaSystemInstructionType | SolanaTokenInstructionType
  params: BraveWallet.SolanaInstructionParam[]
  data: number[]
}

export const getSolanaTransactionInstructionParamsAndType = ({
  programId,
  decodedData,
  accountMetas,
  data,
}: BraveWallet.SolanaInstruction): TypedSolanaInstructionWithParams => {
  // the signers are the `accountMetas` from this index to the end of the array
  // its possible to have any number of signers, including 0
  const accountParams: BraveWallet.SolanaInstructionParam[] = (
    decodedData?.accountParams || []
  ).map(({ localizedName, name }, i) => {
    const isSignersParam = name === BraveWallet.SIGNERS

    return {
      name,
      localizedName,
      type: isSignersParam
        ? BraveWallet.SolanaInstructionParamType.kString
        : BraveWallet.SolanaInstructionParamType.kPublicKey,
      // add a comma separated list of signers as a value if param name is "signers"
      value: isSignersParam
        ? accountMetas.slice(i).join(',')
        : accountMetas[i]?.pubkey
    }
  })

  const typedInstruction: TypedSolanaInstructionWithParams = {
    programId,
    accountParams: accountParams,
    params: decodedData?.params || [],
    type: undefined,
    accountMetas,
    data
  }

  if (decodedData === undefined) {
    // return early if nothing to decode
    return typedInstruction
  }

  switch (programId) {
    case BraveWallet.SOLANA_SYSTEM_PROGRAM_ID: {
      typedInstruction.type =
        SolanaSystemInstructionKeys[decodedData.instructionType]
      break
    }
    case BraveWallet.SOLANA_TOKEN_PROGRAM_ID: {
      typedInstruction.type =
        SolanaTokenInstructionKeys[decodedData.instructionType]
      break
    }
  }

  return typedInstruction
}

export const getTypedSolanaTxInstructions = (solTxData?: SerializableSolanaTxData | BraveWallet.SolanaTxData): TypedSolanaInstructionWithParams[] => {
  const instructions: TypedSolanaInstructionWithParams[] = (solTxData?.instructions || []).map((instruction) => {
    return getSolanaTransactionInstructionParamsAndType(instruction)
  })
  return instructions || []
}

/**
 * formatted if possible:
 *
 * lamports -> SOL
 *
 * pubkey -> friendly account address name
 */
export const formatSolInstructionParamValue = (
  { name, value, type }: BraveWallet.SolanaInstructionParam,
  accounts: Array<{
    address: string
    name: string
  }>
): {
  valueType: 'lamports' | 'address' | 'other'
  formattedValue: string
} => {
  const isAddressParam =
    type === BraveWallet.SolanaInstructionParamType.kOptionalPublicKey ||
    type === BraveWallet.SolanaInstructionParamType.kPublicKey

  const isLamportsParam = name === BraveWallet.LAMPORTS

  const formattedParamValue = (
    isLamportsParam
      ? lamportsToSol(value).formatAsAsset(9, 'SOL')
      : isAddressParam
      ? findAccountName(accounts, value) ?? value
      : value
  ).toString()

  return {
    formattedValue: formattedParamValue,
    valueType: isAddressParam
      ? 'address'
      : isLamportsParam
      ? 'lamports'
      : 'other'
  }
}

export const getSolInstructionAccountParamsObj = (
  accountParams: BraveWallet.SolanaInstructionAccountParam[],
  accountMetas: BraveWallet.SolanaAccountMeta[]
) => {
  let fromAccount: string = ''
  let toAccount: string = ''
  let nonceAccount: string = ''
  let newAccount: string = ''

  accountParams.forEach(({ name }, i) => {
    // Do not show account public key of address table lookup account because
    // it might give the wrong impression to users. For example, users might
    // think they're sending funds to this address table lookup account, but
    // actually they're sending to the account pointed by the index in this
    // address table lookup account.
    const isAddrTableLookupAccount = accountMetas[i]?.addrTableLookupIndex
    const value = isAddrTableLookupAccount ? '' : accountMetas[i]?.pubkey

    switch (name) {
      case BraveWallet.FROM_ACCOUNT: {
        fromAccount = value
        break
      }
      case BraveWallet.TO_ACCOUNT: {
        toAccount = value
        break
      }
      case BraveWallet.NONCE_ACCOUNT: {
        nonceAccount = value
        break
      }
      case BraveWallet.NEW_ACCOUNT: {
        newAccount = value
        break
      }
      default: break
    }
  })

  return {
    fromAccount,
    toAccount,
    nonceAccount,
    newAccount
  }
}

export const getSolInstructionParamsObj = (
  params: BraveWallet.SolanaInstructionParam[]
) => {
  let lamports: string = '0'

  params.forEach(({ name, value }, i) => {
    switch (name) {
      case BraveWallet.LAMPORTS: {
        lamports = value ?? '0'
        break
      }
      default: break
    }
  })

  return {
    lamports
  }
}
