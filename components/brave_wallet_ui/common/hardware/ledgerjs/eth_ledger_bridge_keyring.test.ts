/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { BraveWallet } from '../../../constants/types'
import LedgerBridgeKeyring from './eth_ledger_bridge_keyring'
import { SignatureVRS } from '../../hardware_operations'
import { LedgerDerivationPaths } from '../types'

// class MockApp {
//   signature: SignatureVRS

//   async getAddress (path: string) {
//     return { address: `address for ${path}` }
//   }

//   async signPersonalMessage (path: string, message: Buffer) {
//     return Promise.resolve(this.signature)
//   }

//   async signEIP712HashedMessage (path: string, domainSeparatorHex: string, hashStructMessageHex: string) {
//     return Promise.resolve(this.signature)
//   }
// }

// const createLedgerKeyring = (app: MockApp = new MockApp()) => {
//   const ledgerHardwareKeyring = new LedgerBridgeKeyring()
//   ledgerHardwareKeyring.unlock = async () => {
//     ledgerHardwareKeyring.app = app
//     ledgerHardwareKeyring.deviceId = 'device1'
//     return { success: true }
//   }
//   return ledgerHardwareKeyring
// }

// const unlockedLedgerKeyring = () => {
//   const ledgerHardwareKeyring = new LedgerBridgeKeyring()
//   ledgerHardwareKeyring.app = new MockApp()
//   ledgerHardwareKeyring.deviceId = 'device1'
//   return ledgerHardwareKeyring
// }

// test('Extracting accounts from device', () => {
//   return expect(createLedgerKeyring().getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive))
//     .resolves.toStrictEqual({
//       payload: [
//         {
//           'address': 'address for m/44\'/60\'/0\'/0/0',
//           'derivationPath': 'm/44\'/60\'/0\'/0/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH,
//           'network': undefined
//         },
//         {
//           'address': 'address for m/44\'/60\'/1\'/0/0',
//           'derivationPath': 'm/44\'/60\'/1\'/0/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH,
//           'network': undefined
//         }],
//       success: true
//     }
//     )
// })

// test('Extracting accounts from legacy device', () => {
//   return expect(createLedgerKeyring().getAccounts(-2, 1, LedgerDerivationPaths.Legacy))
//     .resolves.toStrictEqual({
//       payload: [
//         {
//           'address': 'address for m/44\'/60\'/0\'/0',
//           'derivationPath': 'm/44\'/60\'/0\'/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH,
//           'network': undefined
//         },
//         {
//           'address': 'address for m/44\'/60\'/0\'/1',
//           'derivationPath': 'm/44\'/60\'/0\'/1',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH,
//           'network': undefined
//         }],
//       success: true
//     }
//     )
// })

// test('Extracting accounts with deprecated derivation paths', () => {
//   return expect(createLedgerKeyring().getAccounts(-2, 1, LedgerDerivationPaths.Deprecated))
//     .resolves.toStrictEqual({
//       payload: [
//         {
//           'address': 'address for m/44\'/60\'/0\'/0',
//           'derivationPath': 'm/44\'/60\'/0\'/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH,
//           'network': undefined
//         },
//         {
//           'address': 'address for m/44\'/60\'/1\'/0',
//           'derivationPath': 'm/44\'/60\'/1\'/0',
//           'hardwareVendor': 'Ledger',
//           'name': 'Ledger',
//           'deviceId': 'device1',
//           'coin': BraveWallet.CoinType.ETH,
//           'network': undefined
//         }],
//       success: true
//     }
//     )
// })

// test('Check ledger bridge type', () => {
//   const ledgerHardwareKeyring = new LedgerBridgeKeyring()
//   return expect(ledgerHardwareKeyring.type()).toStrictEqual(BraveWallet.LEDGER_HARDWARE_VENDOR)
// })

// test('Check locks for device app only', () => {
//   const ledgerHardwareKeyring = new LedgerBridgeKeyring()
//   expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
//   ledgerHardwareKeyring.app = new MockApp()
//   expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
// })

// test('Check locks for device app and device id', () => {
//   const ledgerHardwareKeyring = new LedgerBridgeKeyring()
//   expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(false)
//   ledgerHardwareKeyring.app = new MockApp()
//   ledgerHardwareKeyring.deviceId = 'test'
//   expect(ledgerHardwareKeyring.isUnlocked()).toStrictEqual(true)
// })

// test('Extract accounts from locked device', () => {
//   const ledgerHardwareKeyring = new LedgerBridgeKeyring()
//   ledgerHardwareKeyring.unlock = async function () {
//     return { success: false, error: 'braveWalletUnlockError' }
//   }
//   return expect(ledgerHardwareKeyring.getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive))
//     .resolves.toStrictEqual({ error: 'braveWalletUnlockError', success: false })
// })

// test('Extract accounts from unknown device', () => {
//   const ledgerHardwareKeyring = unlockedLedgerKeyring()
//   return expect(ledgerHardwareKeyring.getAccounts(-2, 1, 'unknown'))
//     .rejects.toThrow()
// })

// test('Sign personal message successfully with padding v<27', () => {
//   const ledgerHardwareKeyring = unlockedLedgerKeyring()
//   ledgerHardwareKeyring.app.signature = { v: 0, r: 'b68983', s: 'r68983' }
//   return expect(ledgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898300', success: true })
// })

// test('Sign personal message successfully with padding v>=27', () => {
//   const ledgerHardwareKeyring = unlockedLedgerKeyring()
//   ledgerHardwareKeyring.app.signature = { v: 28, r: 'b68983', s: 'r68983' }
//   return expect(ledgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898301', success: true })
// })

// test('Sign personal message successfully without padding v>=27', () => {
//   const ledgerHardwareKeyring = unlockedLedgerKeyring()
//   ledgerHardwareKeyring.app.signature = { v: 44, r: 'b68983', s: 'r68983' }
//   return expect(ledgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898311', success: true })
// })

// test('Sign personal message successfully without padding v<27', () => {
//   const ledgerHardwareKeyring = unlockedLedgerKeyring()
//   ledgerHardwareKeyring.app.signature = { v: 17, r: 'b68983', s: 'r68983' }
//   return expect(ledgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898311', success: true })
// })

// test('Sign personal message failed', () => {
//   const ledgerHardwareKeyring = createLedgerKeyring()
//   return expect(ledgerHardwareKeyring.signPersonalMessage(
//     'm/44\'/60\'/0\'/0/0', 'message'))
//     .resolves.toMatchObject({ success: false })
// })

// test('Sign typed message success', () => {
//   const app = new MockApp()
//   app.signature = { v: 28, r: 'b68983', s: 'r68983' }
//   const ledgerHardwareKeyring = createLedgerKeyring(app)
//   return expect(ledgerHardwareKeyring.signEip712Message(
//     'm/44\'/60\'/0\'/0/0', 'domainSeparatorHex', 'hashStructMessageHex'))
//     .resolves.toStrictEqual({ payload: '0xb68983r6898301', success: true })
// })

// test('Sign typed message locked', () => {
//   const ledgerHardwareKeyring = new LedgerBridgeKeyring()
//   ledgerHardwareKeyring.unlock = async () => {
//     return { success: false }
//   }
//   return expect(ledgerHardwareKeyring.signEip712Message(
//     'm/44\'/60\'/0\'/0/0', 'domainSeparatorHex', 'hashStructMessageHex'))
//     .resolves.toStrictEqual({ success: false })
// })

// test('Sign typed message error', () => {
//   const app = new MockApp()
//   app.signEIP712HashedMessage = async (path: string,
//     domainSeparatorHex: string,
//     hashStructMessageHex: string) => {
//       throw Error('some error')
//   }
//   const ledgerHardwareKeyring = createLedgerKeyring(app)
//   return expect(ledgerHardwareKeyring.signEip712Message(
//     'm/44\'/60\'/0\'/0/0', 'domainSeparatorHex', 'hashStructMessageHex'))
//     .resolves.toStrictEqual({ success: false, error: 'some error', code: 'Error' })
// })
