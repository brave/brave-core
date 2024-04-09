// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  HardwareWalletConnectOpts //
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { BraveWallet } from '../../constants/types'

// Utils
import getAPIProxy from './bridge'
import { getHardwareKeyring } from '../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  LedgerDerivationPaths,
  SolDerivationPaths,
  TrezorDerivationPaths
} from '../hardware/types'
import EthereumLedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import FilecoinLedgerBridgeKeyring from '../hardware/ledgerjs/fil_ledger_bridge_keyring'

export const onConnectHardwareWallet = (
  opts: HardwareWalletConnectOpts
): Promise<BraveWallet.HardwareWalletAccount[]> => {
  return new Promise(async (resolve, reject) => {
    const keyring = getHardwareKeyring(
      opts.hardware,
      opts.coin,
      opts.onAuthorized
    )
    const isLedger = keyring instanceof EthereumLedgerBridgeKeyring
    const isTrezor = keyring instanceof TrezorBridgeKeyring
    if ((isLedger || isTrezor) && opts.scheme) {
      const promise = isLedger
        ? keyring.getAccounts(
            opts.startIndex,
            opts.stopIndex,
            opts.scheme as LedgerDerivationPaths
          )
        : keyring.getAccounts(
            opts.startIndex,
            opts.stopIndex,
            opts.scheme as TrezorDerivationPaths
          )

      promise
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    } else if (keyring instanceof FilecoinLedgerBridgeKeyring && opts.network) {
      keyring
        .getAccounts(opts.startIndex, opts.stopIndex, opts.network)
        .then((result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    } else if (
      keyring instanceof SolanaLedgerBridgeKeyring &&
      opts.network &&
      opts.scheme
    ) {
      keyring
        .getAccounts(
          opts.startIndex,
          opts.stopIndex,
          opts.scheme as SolDerivationPaths
        )
        .then(async (result: GetAccountsHardwareOperationResult) => {
          if (result.payload) {
            const { braveWalletService } = getAPIProxy()
            const addressesEncoded = await braveWalletService.base58Encode(
              result.payload.map((hardwareAccount) => [
                ...(hardwareAccount.addressBytes || [])
              ])
            )
            for (let i = 0; i < result.payload.length; i++) {
              result.payload[i].address = addressesEncoded.addresses[i]
            }
            return resolve(result.payload)
          }
          reject(result.error)
        })
        .catch(reject)
    }
  })
}
