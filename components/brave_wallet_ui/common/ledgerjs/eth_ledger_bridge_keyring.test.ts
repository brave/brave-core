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
  LEDGER_HARDWARE_VENDOR,
  SignatureVRS
} from '../../constants/types'

class MockApp {
  signature: SignatureVRS

  async getAddress (path: string) {
    return { address: `address for ${path}` }
  }

  async signPersonalMessage (path: string, message: Buffer) {
    return Promise.resolve(this.signature)
  }
}

const createLedgerKeyring = () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  ledgerHardwareKeyring.unlock = async () => {
    ledgerHardwareKeyring.app = new MockApp()
    ledgerHardwareKeyring.deviceId_ = 'device1'
    return true
  }
  return ledgerHardwareKeyring
}

test('Extracting accounts from device', () => {
  return expect(createLedgerKeyring().getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive))
    .resolves.toStrictEqual([
      {
        'address': 'address for m/44\'/60\'/0\'/0/0',
        'derivationPath': 'm/44\'/60\'/0\'/0/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger',
        'deviceId': 'device1'
      },
      {
        'address': 'address for m/44\'/60\'/1\'/0/0',
        'derivationPath': 'm/44\'/60\'/1\'/0/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger',
        'deviceId': 'device1'
      }]
    )
})

test('Extracting accounts from legacy device', () => {
  return expect(createLedgerKeyring().getAccounts(-2, 1, LedgerDerivationPaths.Legacy))
    .resolves.toStrictEqual([
      {
        'address': 'address for m/44\'/60\'/0\'/0',
        'derivationPath': 'm/44\'/60\'/0\'/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger',
        'deviceId': 'device1'
      },
      {
        'address': 'address for m/44\'/60\'/1\'/0',
        'derivationPath': 'm/44\'/60\'/1\'/0',
        'hardwareVendor': 'Ledger',
        'name': 'Ledger',
        'deviceId': 'device1'
      }]
    )
})

test('Check ledger bridge type', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  return expect(ledgerHardwareKeyring.type()).toStrictEqual(LEDGER_HARDWARE_VENDOR)
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

test('Sign personal message successfully', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  ledgerHardwareKeyring.app = new MockApp()
  ledgerHardwareKeyring.app.signature = { v: 1, r: 'b68983', s: 'r68983' }
  ledgerHardwareKeyring._recoverAddressFromSignature = (message: string, signature: string) => {
    return '0x111'
  }
  return expect(ledgerHardwareKeyring.signPersonalMessage(
    'm/44\'/60\'/0\'/0/0', '0x111', 'message'))
    .resolves.toStrictEqual('0xb68983r68983-26')
})

test('Sign personal message failed', () => {
  const ledgerHardwareKeyring = new LedgerBridgeKeyring()
  ledgerHardwareKeyring.app = new MockApp()
  ledgerHardwareKeyring._recoverAddressFromSignature = (message: string, signature: string) => {
    return '0x111'
  }
  return expect(ledgerHardwareKeyring.signPersonalMessage(
    'm/44\'/60\'/0\'/0/0', '0x111', 'message'))
    .rejects.toThrow()
})
