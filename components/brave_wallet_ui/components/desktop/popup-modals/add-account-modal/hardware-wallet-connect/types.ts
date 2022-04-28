/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { FilecoinNetwork, HardwareDerivationScheme, LedgerDerivationPaths, TrezorDerivationPaths } from '../../../../../common/hardware/types'
import { BraveWallet } from '../../../../../constants/types'
import { HardwareVendor } from '../../../../../common/api/hardware_keyrings'

export const HardwareWalletDerivationPathLocaleMapping = {
  [LedgerDerivationPaths.LedgerLive]: 'Ledger Live',
  [LedgerDerivationPaths.Legacy]: 'Legacy (MEW/MyCrypto)',
  [LedgerDerivationPaths.Deprecated]: 'Deprecated (Not recommended)',
  [TrezorDerivationPaths.Default]: 'Default'
}

export const HardwareWalletDerivationPathsMapping = {
  [BraveWallet.LEDGER_HARDWARE_VENDOR]: LedgerDerivationPaths,
  [BraveWallet.TREZOR_HARDWARE_VENDOR]: TrezorDerivationPaths
}

export interface HardwareWalletConnectOpts {
  hardware: HardwareVendor
  // TODO: add currency and network as enums
  // currency: string
  // network: string

  startIndex: number
  stopIndex: number
  scheme?: HardwareDerivationScheme
  network?: FilecoinNetwork
  coin: BraveWallet.CoinType
}

export interface ErrorMessage {
  error: string
  userHint: string
}
