/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { assert } from 'chrome://resources/js/assert.m.js'
import { BraveWallet } from '../../constants/types'
import LedgerBridgeKeyring from '../../common/hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../../common/hardware/trezor/trezor_bridge_keyring'
import * as HWInterfaces from '../hardware/interfaces'
import FilecoinLedgerKeyring from '../hardware/ledgerjs/filecoin_ledger_keyring'

export type HardwareKeyring = LedgerBridgeKeyring | TrezorBridgeKeyring

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
let ethereumHardwareKeyring: LedgerBridgeKeyring
let filecoinHardwareKeyring: FilecoinLedgerKeyring
let trezorHardwareKeyring: TrezorBridgeKeyring
let keyringService: BraveWallet.KeyringServiceRemote

export function getBraveKeyring (): BraveWallet.KeyringServiceRemote {
  if (!keyringService) {
    /** @type {!braveWallet.mojom.KeyringServiceRemote} */
    keyringService = new BraveWallet.KeyringServiceRemote()
  }
  return keyringService
}

export function getHardwareKeyring (type: HardwareVendor, coin: BraveWallet.CoinType = BraveWallet.CoinType.ETH): LedgerBridgeKeyring | HWInterfaces.TrezorKeyring | FilecoinLedgerKeyring {
  if (type === BraveWallet.LEDGER_HARDWARE_VENDOR) {
    if (coin === BraveWallet.CoinType.ETH) {
      return getLedgerEthereumHardwareKeyring()
    } else if (coin === BraveWallet.CoinType.FIL) {
      return getLedgerFilecoinHardwareKeyring()
    }
  }

  const trezorKeyring = getTrezorHardwareKeyring()
  assert(type === trezorHardwareKeyring.type())
  return trezorKeyring
}

export function getLedgerEthereumHardwareKeyring (): LedgerBridgeKeyring {
  if (!ethereumHardwareKeyring) {
    ethereumHardwareKeyring = new LedgerBridgeKeyring()
  }
  return ethereumHardwareKeyring
}

export function getLedgerFilecoinHardwareKeyring (): FilecoinLedgerKeyring {
  if (!filecoinHardwareKeyring) {
    filecoinHardwareKeyring = new FilecoinLedgerKeyring()
  }
  return filecoinHardwareKeyring
}

export function getTrezorHardwareKeyring (): TrezorBridgeKeyring {
  if (!trezorHardwareKeyring) {
    trezorHardwareKeyring = new TrezorBridgeKeyring()
  }
  return trezorHardwareKeyring
}
