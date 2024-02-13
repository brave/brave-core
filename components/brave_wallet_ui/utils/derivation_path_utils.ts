// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// utils
import { assert } from 'chrome://resources/js/assert.js'
import { getLocale } from '../../common/locale'

// types
import { FilecoinNetwork, BraveWallet } from '../constants/types'
import {
  LedgerDerivationPaths,
  SolDerivationPaths,
  TrezorDerivationPaths
} from '../common/hardware/types'

export const getPathForEthLedgerIndex = (
  index: number | undefined,
  scheme: LedgerDerivationPaths
): string => {
  if (scheme === LedgerDerivationPaths.LedgerLive) {
    return `m/44'/60'/${index ?? 'x'}'/0/0`
  }
  if (scheme === LedgerDerivationPaths.Deprecated) {
    return `m/44'/60'/${index ?? 'x'}'/0`
  }

  assert(scheme === LedgerDerivationPaths.Legacy, '')
  return `m/44'/60'/0'/${index ?? 'x'}`
}

export const getPathForFilLedgerIndex = (
  index: number | undefined,
  type: FilecoinNetwork
): string => {
  // According to SLIP-0044 For TEST networks coin type use 1 always.
  // https://github.com/satoshilabs/slips/blob/5f85bc4854adc84ca2dc5a3ab7f4b9e74cb9c8ab/slip-0044.md
  // https://github.com/glifio/modules/blob/primary/packages/filecoin-wallet-provider/src/utils/createPath/index.ts
  return type === BraveWallet.FILECOIN_MAINNET
    ? `m/44'/461'/0'/0/${index ?? 'x'}`
    : `m/44'/1'/0'/0/${index ?? 'x'}`
}

export const getPathForSolLedgerIndex = (
  index: number | undefined,
  scheme: SolDerivationPaths
): string => {
  if (scheme === SolDerivationPaths.Bip44Root) {
    return `44'/501'`
  }
  if (scheme === SolDerivationPaths.LedgerLive) {
    return `44'/501'/${index ?? 'x'}'`
  }
  assert(scheme === SolDerivationPaths.Default, '')

  return `44'/501'/${index ?? 'x'}'/0'`
}

export const getPathForTrezorIndex = (
  index: number | undefined,
  scheme: TrezorDerivationPaths
) => {
  if (scheme === TrezorDerivationPaths.Default) {
    return `m/44'/60'/0'/0/${index ?? 'x'}`
  } else {
    throw Error(getLocale('braveWalletDeviceUnknownScheme'))
  }
}
