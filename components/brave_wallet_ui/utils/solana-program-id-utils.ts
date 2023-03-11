// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../constants/types'

// utils
import { getLocale } from '$web-common/locale'

/**
 * see: https://docs.solana.com/developing/runtime-facilities/programs
 * @param programId Solana Program public key
 * @returns Name of program
 */
export const getSolanaProgramIdName = (programId: string): string => {
  switch (programId) {
    case BraveWallet.SOLANA_SYSTEM_PROGRAM_ID:
      return getLocale('braveWalletSolanaSystemProgram')
    case BraveWallet.SOLANA_CONFIG_PROGRAM_ID:
      return getLocale('braveWalletSolanaConfigProgram')
    case BraveWallet.SOLANA_STAKE_PROGRAM_ID:
      return getLocale('braveWalletSolanaStakeProgram')
    case BraveWallet.SOLANA_VOTE_PROGRAM_ID:
      return getLocale('braveWalletSolanaVoteProgram')
    case BraveWallet.SOLANA_BPF_LOADER_UPGRADEABLE_PROGRAM_ID:
      return getLocale('braveWalletSolanaBPFLoader')
    case BraveWallet.SOLANA_ED25519_SIG_VERIFY_PROGRAM_ID:
      return getLocale('braveWalletSolanaEd25519Program')
    case BraveWallet.SOLANA_KECCAK_SECP256K_PROGRAM_ID:
      return getLocale('braveWalletSolanaSecp256k1Program')
    case BraveWallet.SOLANA_ASSOCIATED_TOKEN_PROGRAM_ID:
      return getLocale('braveWalletSolanaAssociatedTokenProgram')
    case BraveWallet.SOLANA_METADATA_PROGRAM_ID:
      return getLocale('braveWalletSolanaMetaDataProgram')
    case BraveWallet.SOLANA_SYSVAR_RENT_PROGRAM_ID:
      return getLocale('braveWalletSolanaSysvarRentProgram')
    case BraveWallet.SOLANA_TOKEN_PROGRAM_ID:
      return getLocale('braveWalletSolanaTokenProgram')
    default:
      return programId
  }
}
