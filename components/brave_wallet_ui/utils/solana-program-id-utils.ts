// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as Solana from '@solana/web3.js'

// utils
import { getLocale } from '$web-common/locale'

/**
 * see: https://docs.solana.com/developing/runtime-facilities/programs
 * @param programId Solana.PublicKey
 * @returns Name of program
 */
export const getSolanaProgramIdName = (programId: Solana.PublicKey) => {
  switch (programId.toString()) {
    case '11111111111111111111111111111111': return getLocale('braveWalletSolanaSystemProgram')
    case 'Config1111111111111111111111111111111111111': return getLocale('braveWalletSolanaConfigProgram')
    case 'Stake11111111111111111111111111111111111111': return getLocale('braveWalletSolanaStakeProgram')
    case 'Vote111111111111111111111111111111111111111': return getLocale('braveWalletSolanaVoteProgram')
    case 'BPFLoaderUpgradeab1e11111111111111111111111': return getLocale('braveWalletSolanaBPFLoader')
    case 'Ed25519SigVerify111111111111111111111111111': return getLocale('braveWalletSolanaEd25519Program')
    case 'KeccakSecp256k11111111111111111111111111111': return getLocale('braveWalletSolanaSecp256k1Program')
    default: return programId.toString()
  }
}
