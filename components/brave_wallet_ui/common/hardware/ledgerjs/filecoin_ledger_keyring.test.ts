/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import { TextEncoder, TextDecoder } from 'util'
global.TextDecoder = TextDecoder
global.TextEncoder = TextEncoder
import { LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import FilecoinLedgerKeyring from './filecoin_ledger_keyring'
import { CoinType } from '@glif/filecoin-address'
import { LedgerProvider } from '@glif/filecoin-wallet-provider'
import { BraveWallet } from '../../../constants/types'

class MockApp {
  async getAccounts (from: number, to: number, coinType?: CoinType): Promise<string[]> {
    let result = []
    for (let i = from; i < to; i++) {
      result.push(i.toString())
    }
    return result
  }
}

const createFilecoinKeyring = () => {
  const ledgerHardwareKeyring = new FilecoinLedgerKeyring()
  ledgerHardwareKeyring.unlock = async () => {
    ledgerHardwareKeyring.provider = new MockApp() as LedgerProvider
    ledgerHardwareKeyring.deviceId = 'device1'
    return { success: true }
  }
  return ledgerHardwareKeyring
}

test('Extracting accounts from device', () => {
  return expect(createFilecoinKeyring().getAccounts(-2, 1, CoinType.TEST))
    .resolves.toStrictEqual({
      payload: [{
        address: '0',
        coin: BraveWallet.CoinType.FIL,
        derivationPath: 'm/44\'/461\'/0\'/0/0',
        deviceId: 'device1',
        hardwareVendor: 'Ledger',
        name: 'Filecoin Ledger'
      }],
      success: true
    })
})

test('Check ledger bridge type', () => {
  const ledgerHardwareKeyring = new FilecoinLedgerKeyring()
  return expect(ledgerHardwareKeyring.type()).toStrictEqual(LEDGER_HARDWARE_VENDOR)
})

test('Check locks for device', () => {
  const ledgerHardwareKeyring = new FilecoinLedgerKeyring()
  expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
  ledgerHardwareKeyring.provider = new MockApp() as LedgerProvider
  expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(true)
})

test('Extract accounts from locked device', () => {
  const ledgerHardwareKeyring = new FilecoinLedgerKeyring()
  ledgerHardwareKeyring.unlock = async function () {
    return { success: false, error: 'braveWalletUnlockError' }
  }
  return expect(ledgerHardwareKeyring.getAccounts(-2, 1, CoinType.TEST))
    .resolves.toStrictEqual({ error: 'braveWalletUnlockError', success: false })
})
