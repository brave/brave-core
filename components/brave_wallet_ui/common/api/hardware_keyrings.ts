/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assertNotReached } from 'chrome://resources/js/assert.js'
import { BraveWallet } from '../../constants/types'

import * as HWInterfaces from '../hardware/interfaces'
import EthereumLedgerBridgeKeyring from '../../common/hardware/ledgerjs/eth_ledger_bridge_keyring'
import SolanaLedgerBridgeKeyring from '../../common/hardware/ledgerjs/sol_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../../common/hardware/trezor/trezor_bridge_keyring'
import {
  createTrezorBridge,
  TrezorBridgeTransport,
} from '../../common/hardware/trezor/trezor-bridge-transport'
import { kTrezorBridgeUrl } from '../../common/hardware/trezor/trezor-messages'
import FilecoinLedgerBridgeKeyring from '../../common/hardware/ledgerjs/fil_ledger_bridge_keyring'
import BitcoinLedgerBridgeKeyring from '../hardware/ledgerjs/btc_ledger_bridge_keyring'

// Lazy instances for keyrings
let ethereumHardwareKeyring: EthereumLedgerBridgeKeyring
let filecoinHardwareKeyring: FilecoinLedgerBridgeKeyring
let solanaHardwareKeyring: SolanaLedgerBridgeKeyring
let bitcoinHardwareKeyring: BitcoinLedgerBridgeKeyring
let trezorHardwareKeyring: TrezorBridgeKeyring

export async function getHardwareKeyring(
  vendor: BraveWallet.HardwareVendor,
  coin: BraveWallet.CoinType,
  onAuthorized?: () => void,
): Promise<
  | EthereumLedgerBridgeKeyring
  | HWInterfaces.TrezorKeyring
  | FilecoinLedgerBridgeKeyring
  | SolanaLedgerBridgeKeyring
  | BitcoinLedgerBridgeKeyring
> {
  if (vendor === BraveWallet.HardwareVendor.kLedger) {
    if (coin === BraveWallet.CoinType.ETH) {
      return getLedgerEthereumHardwareKeyring(onAuthorized)
    } else if (coin === BraveWallet.CoinType.FIL) {
      return getLedgerFilecoinHardwareKeyring(onAuthorized)
    } else if (coin === BraveWallet.CoinType.SOL) {
      return getLedgerSolanaHardwareKeyring(onAuthorized)
    } else if (coin === BraveWallet.CoinType.BTC) {
      return getLedgerBitcoinHardwareKeyring(onAuthorized)
    }
  } else if (vendor === BraveWallet.HardwareVendor.kTrezor) {
    if (coin === BraveWallet.CoinType.ETH) {
      return await getTrezorHardwareKeyring()
    }
  }

  assertNotReached(`Unsupported coin ${coin} and vendor ${vendor}`)
}

export function getLedgerEthereumHardwareKeyring(
  onAuthorized?: () => void,
): EthereumLedgerBridgeKeyring {
  if (!ethereumHardwareKeyring) {
    ethereumHardwareKeyring = new EthereumLedgerBridgeKeyring(onAuthorized)
  }
  return ethereumHardwareKeyring
}

export function getLedgerFilecoinHardwareKeyring(
  onAuthorized?: () => void,
): FilecoinLedgerBridgeKeyring {
  if (!filecoinHardwareKeyring) {
    filecoinHardwareKeyring = new FilecoinLedgerBridgeKeyring(onAuthorized)
  }
  return filecoinHardwareKeyring
}

export function getLedgerSolanaHardwareKeyring(
  onAuthorized?: () => void,
): SolanaLedgerBridgeKeyring {
  if (!solanaHardwareKeyring) {
    solanaHardwareKeyring = new SolanaLedgerBridgeKeyring(onAuthorized)
  }
  return solanaHardwareKeyring
}

export function getLedgerBitcoinHardwareKeyring(
  onAuthorized?: () => void,
): BitcoinLedgerBridgeKeyring {
  if (!bitcoinHardwareKeyring) {
    bitcoinHardwareKeyring = new BitcoinLedgerBridgeKeyring(onAuthorized)
  }
  return bitcoinHardwareKeyring
}

export async function getTrezorHardwareKeyring(): Promise<TrezorBridgeKeyring> {
  if (!trezorHardwareKeyring) {
    const bridge = await createTrezorBridge(kTrezorBridgeUrl)
    trezorHardwareKeyring = new TrezorBridgeKeyring(
      new TrezorBridgeTransport(kTrezorBridgeUrl, bridge),
    )
  }
  return trezorHardwareKeyring
}
