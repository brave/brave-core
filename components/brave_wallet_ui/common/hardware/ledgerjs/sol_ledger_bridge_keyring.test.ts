/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { BraveWallet } from '../../../constants/types'
import SolanaLedgerBridgeKeyring from './sol_ledger_bridge_keyring'

class MockApp {
  signature: Buffer

  async getAddress (path: string) {
    return { address: Buffer.from(`address for ${path}`) }
  }

  async signTransaction (path: string, txBuffer: Buffer) {
    return { signature: Buffer.from('signature') }
  }
}

const createSolanaLedgerKeyring = (app: MockApp = new MockApp()) => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerBridgeKeyring()
  solanaLedgerHardwareKeyring.unlock = async () => {
    solanaLedgerHardwareKeyring.app = app
    solanaLedgerHardwareKeyring.deviceId = 'device1'
    return { success: true }
  }

  return solanaLedgerHardwareKeyring
}

test('Extracting accounts from device', () => {
  return expect(createSolanaLedgerKeyring().getAccounts(-2, 1))
    .resolves.toStrictEqual({
      payload: [
        {
          'address': '',
          'addressBytes': Buffer.from('address for 44\'/501\'/0\'/0\''),
          'derivationPath': '44\'/501\'/0\'/0\'',
          'hardwareVendor': 'Ledger',
          'name': 'Ledger',
          'deviceId': 'device1',
          'coin': BraveWallet.CoinType.SOL,
          'network': undefined
        },
        {
          'address': '',
          'addressBytes': Buffer.from('address for 44\'/501\'/1\'/0\''),
          'derivationPath': '44\'/501\'/1\'/0\'',
          'hardwareVendor': 'Ledger',
          'name': 'Ledger',
          'deviceId': 'device1',
          'coin': BraveWallet.CoinType.SOL,
          'network': undefined
        }],
      success: true
    }
    )
})

test('Check ledger bridge type', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerBridgeKeyring()
  return expect(solanaLedgerHardwareKeyring.type()).toStrictEqual(BraveWallet.LEDGER_HARDWARE_VENDOR)
})

test('Check locks for device app only', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerBridgeKeyring()
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
  solanaLedgerHardwareKeyring.app = new MockApp()
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
})

test('Check locks for device app and device id', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerBridgeKeyring()
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
  solanaLedgerHardwareKeyring.app = new MockApp()
  solanaLedgerHardwareKeyring.deviceId = 'test'
  expect(solanaLedgerHardwareKeyring.isUnlocked()).toStrictEqual(true)
})

test('Extract accounts from locked device yields unlock error', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerBridgeKeyring()
  solanaLedgerHardwareKeyring.unlock = async function () {
    return { success: false, error: 'braveWalletUnlockError' }
  }
  return expect(solanaLedgerHardwareKeyring.getAccounts(-2, 1))
    .resolves.toStrictEqual({ error: 'braveWalletUnlockError', success: false })
})

test('Sign transaction from locked device yields unlock error', () => {
  const solanaLedgerHardwareKeyring = new SolanaLedgerBridgeKeyring()
  solanaLedgerHardwareKeyring.unlock = async function () {
    return { success: false, error: 'braveWalletUnlockError' }
  }
  return expect(solanaLedgerHardwareKeyring.signTransaction(1, 'message'))
    .resolves.toStrictEqual({ error: 'braveWalletUnlockError', success: false })
})

test('Sign transaction unlocked device yields success', async () => {
  const ledgerHardwareKeyring = new SolanaLedgerBridgeKeyring()
  ledgerHardwareKeyring.unlock = async function () {
    return { success: true }
  }
  ledgerHardwareKeyring.app = new MockApp() as LedgerProvider
  return expect(ledgerHardwareKeyring.signTransaction(0, 'message'))
    .resolves.toStrictEqual({ success: true, payload: Buffer.from('signature') })
})
