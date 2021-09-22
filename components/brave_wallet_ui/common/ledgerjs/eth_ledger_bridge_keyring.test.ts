/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import LedgerBridgeKeyring from './eth_ledger_bridge_keyring'

import {
  LedgerDerivationPaths
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'

import {
  kLedgerHardwareVendor
} from '../../constants/types'

class MockApp {
  async getAddress (path: string) {
    return { address: `address for ${path}` }
  }
}

test('Extracting accounts from device', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  ledgerHardwareKeyring.app = new MockApp()
  return expect(ledgerHardwareKeyring.getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive))
    .resolves.toStrictEqual([
      {
        'address': 'address for m/44\'/60\'/0\'/0/0',
        'derivationPath': 'm/44\'/60\'/0\'/0/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger 0'
      },
      {
        'address': 'address for m/44\'/60\'/1\'/0/0',
        'derivationPath': 'm/44\'/60\'/1\'/0/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger 1'
      }]
    )
})

test('Extracting accounts from legacy device', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  ledgerHardwareKeyring.app = new MockApp()
  return expect(ledgerHardwareKeyring.getAccounts(-2, 1, LedgerDerivationPaths.Legacy))
    .resolves.toStrictEqual([
      {
        'address': 'address for m/44\'/60\'/0\'/0',
        'derivationPath': 'm/44\'/60\'/0\'/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger 0'
      },
      {
        'address': 'address for m/44\'/60\'/1\'/0',
        'derivationPath': 'm/44\'/60\'/1\'/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger 1'
      }]
    )
})

test('Check ledger bridge type', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  return expect(ledgerHardwareKeyring.type()).toStrictEqual(kLedgerHardwareVendor)
})

test('Check locks for device', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
  ledgerHardwareKeyring.app = new MockApp()
  expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(true)
})

test('Extract accounts from locked device', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  ledgerHardwareKeyring.unlock = async function () {
    return false
  }
  return expect(ledgerHardwareKeyring.getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive))
  .rejects.toThrow()
})

test('Extract accounts from unknown device', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  ledgerHardwareKeyring.app = new MockApp()
  return expect(ledgerHardwareKeyring.getAccounts(-2, 1, 'unknown'))
  .rejects.toThrow()
})
