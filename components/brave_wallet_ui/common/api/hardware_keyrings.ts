/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { assert } from 'chrome://resources/js/assert.m.js'
import { BraveWallet } from '../../constants/types'
import EthereumLedgerBridgeKeyring from '../../common/hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../../common/hardware/trezor/trezor_bridge_keyring'
import * as HWInterfaces from '../hardware/interfaces'
import FilecoinLedgerKeyring from '../hardware/ledgerjs/filecoin_ledger_keyring'
import SolanaLedgerBridgeKeyring from '../../common/hardware/ledgerjs/sol_ledger_bridge_keyring'

export type HardwareKeyring = HWInterfaces.LedgerEthereumKeyring | HWInterfaces.TrezorKeyring | HWInterfaces.LedgerFilecoinKeyring | HWInterfaces.LedgerSolanaKeyring

export function getCoinName (coin: BraveWallet.CoinType) {
  switch (coin) {
    case BraveWallet.CoinType.FIL:
      return 'Filecoin'
    case BraveWallet.CoinType.ETH:
      return 'Ethereum'
  }
  return ''
}

const VendorTypes = [
  BraveWallet.TREZOR_HARDWARE_VENDOR,
  BraveWallet.LEDGER_HARDWARE_VENDOR
] as const
export type HardwareVendor = typeof VendorTypes[number]

// Lazy instances for keyrings
let ethereumHardwareKeyring: EthereumLedgerBridgeKeyring
let filecoinHardwareKeyring: FilecoinLedgerKeyring
let solanaHardwareKeyring: SolanaLedgerBridgeKeyring
let trezorHardwareKeyring: TrezorBridgeKeyring
let keyringService: BraveWallet.KeyringServiceRemote

export function getBraveKeyring (): BraveWallet.KeyringServiceRemote {
  if (!keyringService) {
    /** @type {!braveWallet.mojom.KeyringServiceRemote} */
    keyringService = new BraveWallet.KeyringServiceRemote()
  }
  return keyringService
}

export function getHardwareKeyring (
  type: HardwareVendor,
  coin: BraveWallet.CoinType = BraveWallet.CoinType.ETH,
  onAuthorized?: () => void
): EthereumLedgerBridgeKeyring | HWInterfaces.TrezorKeyring | FilecoinLedgerKeyring | SolanaLedgerBridgeKeyring {
  if (type === BraveWallet.LEDGER_HARDWARE_VENDOR) {
    if (coin === BraveWallet.CoinType.ETH) {
      return getLedgerEthereumHardwareKeyring(onAuthorized)
    } else if (coin === BraveWallet.CoinType.FIL) {
      return getLedgerFilecoinHardwareKeyring()
    } else if (coin === BraveWallet.CoinType.SOL) {
      return getLedgerSolanaHardwareKeyring(onAuthorized)
    }
  }

  const trezorKeyring = getTrezorHardwareKeyring()
  assert(type === trezorHardwareKeyring.type())
  return trezorKeyring
}

export function getLedgerEthereumHardwareKeyring (onAuthorized?: () => void): EthereumLedgerBridgeKeyring {
  if (!ethereumHardwareKeyring) {
    ethereumHardwareKeyring = new EthereumLedgerBridgeKeyring(onAuthorized)
  }
  return ethereumHardwareKeyring
}

export function getLedgerFilecoinHardwareKeyring (): FilecoinLedgerKeyring {
  if (!filecoinHardwareKeyring) {
    filecoinHardwareKeyring = new FilecoinLedgerKeyring()
  }
  return filecoinHardwareKeyring
}

export function getLedgerSolanaHardwareKeyring (onAuthorized?: () => void): SolanaLedgerBridgeKeyring {
  if (!solanaHardwareKeyring) {
    solanaHardwareKeyring = new SolanaLedgerBridgeKeyring(onAuthorized)
  }
  return solanaHardwareKeyring
}

export function getTrezorHardwareKeyring (): TrezorBridgeKeyring {
  if (!trezorHardwareKeyring) {
    trezorHardwareKeyring = new TrezorBridgeKeyring()
  }
  return trezorHardwareKeyring
}
