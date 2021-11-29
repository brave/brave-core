/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { HardwareDerivationScheme, LedgerDerivationPaths, TrezorDerivationPaths } from '../../../../../common/hardware/types'
import { LEDGER_HARDWARE_VENDOR, TREZOR_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { HardwareVendor } from 'components/brave_wallet_ui/common/api/getKeyringsByType'

export const HardwareWalletDerivationPathLocaleMapping = {
  [LedgerDerivationPaths.LedgerLive]: 'Ledger Live',
  [LedgerDerivationPaths.Legacy]: 'Legacy (MEW/MyCrypto)',
  [TrezorDerivationPaths.Default]: 'Default'
}

export const HardwareWalletDerivationPathsMapping = {
  [LEDGER_HARDWARE_VENDOR]: LedgerDerivationPaths,
  [TREZOR_HARDWARE_VENDOR]: TrezorDerivationPaths
}

export interface HardwareWalletConnectOpts {
  hardware: HardwareVendor
  // TODO: add currency and network as enums
  // currency: string
  // network: string

  startIndex: number
  stopIndex: number
  scheme: HardwareDerivationScheme
}

export interface ErrorMessage {
  error: string
  userHint: string
}
