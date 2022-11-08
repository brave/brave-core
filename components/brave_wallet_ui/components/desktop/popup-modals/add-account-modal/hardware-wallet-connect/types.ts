/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { FilecoinNetwork, HardwareDerivationScheme, LedgerDerivationPaths, TrezorDerivationPaths, SolDerivationPaths } from '../../../../../common/hardware/types'
import { BraveWallet } from '../../../../../constants/types'
import { HardwareVendor } from '../../../../../common/api/hardware_keyrings'
export { SolDerivationPaths } from '../../../../../common/hardware/types'

// TODO(apaymyshev): strings below need localization.
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

// TODO(apaymyshev): strings below need localization.
export const SolHardwareWalletDerivationPathLocaleMapping = {
  [SolDerivationPaths.Default]: 'Default',
  [SolDerivationPaths.LedgerLive]: 'Ledger Live'
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
  onAuthorized: () => void
}

export interface ErrorMessage {
  error: string
  userHint: string
}
