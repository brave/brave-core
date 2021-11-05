/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import TrezorBridgeKeyring from './trezor_bridge_keyring'
import {
  TrezorDerivationPaths, TrezorBridgeAccountsPayload
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import {
  kTrezorBridgeUrl,
  TrezorGetAccountsResponse,
  TrezorFrameResponse,
  TrezorCommand,
  TrezorError,
  TrezorBridgeTransport
} from '../../common/trezor/trezor-messages'
import {
  kTrezorHardwareVendor
} from '../../constants/types'
import { getLocale } from '../../../common/locale'

let uuid = 0
window.crypto = {
  randomUUID () {
    return uuid++
  }
}

const createTrezorTransport = (unlock: Boolean,
                             accounts?: TrezorGetAccountsResponse) => {
  const hardwareTransport = new TrezorBridgeTransport()
  hardwareTransport.windowListeners_ = {}
  hardwareTransport.getTrezorBridgeOrigin = () => {
    return 'braveWalletTrezorBridgeUrl'
  }
  hardwareTransport.addWindowMessageListener = () => {
    hardwareTransport.expectWindowMessageSubscribers(0)

    const key = hardwareTransport.onMessageReceived.toString()
    hardwareTransport.windowListeners_[key] =
    hardwareTransport.onMessageReceived

    hardwareTransport.expectWindowMessageSubscribers(1)
  }
  hardwareTransport.expectWindowMessageSubscribers = (amount: number) => {
    const keys = Object.keys(hardwareTransport.windowListeners_)
    expect(keys.length).toStrictEqual(amount)
  }
  hardwareTransport.removeWindowMessageListener = () => {
    hardwareTransport.expectWindowMessageSubscribers(1)
    const key = hardwareTransport.onMessageReceived.toString()
    delete hardwareTransport.windowListeners_[key]
    hardwareTransport.expectWindowMessageSubscribers(0)
  }

  hardwareTransport.postResponse = (data: TrezorFrameResponse) => {
    for (const value of Object.values(hardwareTransport.windowListeners_)) {
      (value as Function)(
        { type: 'message',
          origin: kTrezorBridgeUrl,
          data: data
        }, kTrezorBridgeUrl)
    }
  }
  hardwareTransport.contentWindow = {
    postMessage: (message: any, command: any) => {
      expect(command).toStrictEqual(kTrezorBridgeUrl)
      if (message.command === TrezorCommand.Unlock) {
        hardwareTransport.postResponse({
          id: message.id,
          command: TrezorCommand.Unlock,
          result: unlock
        })
      }
      if (message.command === TrezorCommand.GetAccounts) {
        hardwareTransport.postResponse({
          id: message.id,
          command: TrezorCommand.GetAccounts,
          payload: accounts
        })
      }
    }
  }
  hardwareTransport.createBridge = async () => {
    return hardwareTransport
  }

  return hardwareTransport
}

test('Wait for responses', () => {
  const transport = createTrezorTransport(false)
  transport.expectWindowMessageSubscribers(0)
  transport.addEventListener('1', (data: any) => {
    expect(data.result).toStrictEqual(true)
  })
  transport.addEventListener('2', (data: any) => {
    expect(data.result).toStrictEqual(true)
  })
  transport.addEventListener('3', (data: any) => {
    expect(data.result).toStrictEqual(false)
  })

  transport.expectWindowMessageSubscribers(1)

  // Unknown id
  transport.postResponse({
    id: '4',
    command: TrezorCommand.Unlock,
    result: true
  })

  expect(transport.pendingRequests.size).toStrictEqual(3)
  transport.postResponse({
    id: '2',
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(transport.pendingRequests.size).toStrictEqual(2)
  transport.postResponse({
    id: '1',
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(transport.pendingRequests.size).toStrictEqual(1)

  // same id twice
  transport.postResponse({
    id: '1',
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(transport.pendingRequests.size).toStrictEqual(1)

  transport.postResponse({
    id: '3',
    command: TrezorCommand.Unlock,
    result: false
  })
  expect(transport.pendingRequests.size).toStrictEqual(0)
  transport.expectWindowMessageSubscribers(0)
})

test('isUnlocked', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(false)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  hardwareKeyring.unlocked_ = true
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(true)
})

test('Unlock device success', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(true)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).resolves.toStrictEqual(true)
})

test('Check trezor bridge type', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(true,
                                              { success: true, payload: [] })
  return expect(hardwareKeyring.type()).toStrictEqual(kTrezorHardwareVendor)
})

test('Unlock device fail', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(false)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).rejects.toStrictEqual(false)
})

test('Extract accounts from locked device', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(false,
                                              { success: true, payload: [] })
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
  .rejects.toStrictEqual(false)
})

test('Extracting accounts from unlocked device fail to access bridge', () => {
  const error: TrezorError = { error: getLocale('braveWalletCreateBridgeError'),
    code: 'code' }
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(true, { success: false, payload: error })
  const expected: TrezorBridgeAccountsPayload = {
    success: false, error: error.error, accounts: []
  }
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
    .rejects.toStrictEqual(expected)
})

test('Extracting accounts from unlocked device returned fail', () => {
  const error: TrezorError = { error: getLocale('braveWalletCreateBridgeError'),
    code: 'code' }
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(true, { success: false, payload: error })
  hardwareKeyring.getBridge = () => {
    return hardwareKeyring as any
  }
  const expected: TrezorBridgeAccountsPayload = {
    success: false, error: error.error, accounts: []
  }
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
    .rejects.toStrictEqual(expected)
})

test('Extracting accounts from unlocked device returned success', () => {
  const accounts = [
    {
      publicKey: '3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9e0dca11a001cf066d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1d',
      serializedPath: 'm/44\'/60\'/0\'/0',
      fingerprint: 5454545
    },
    {
      publicKey: '3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9e0dca11a001cf066d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1d',
      serializedPath: 'm/44\'/60\'/0\'/1',
      fingerprint: 5454545
    }
  ]
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(true, { success: true, payload: accounts })
  hardwareKeyring.getBridge = () => {
    return hardwareKeyring as any
  }
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
    .resolves.toStrictEqual([
      { 'address': '0x2F015C60E0be116B1f0CD534704Db9c92118FB6A',
        'derivationPath': 'm/44\'/60\'/0\'/0',
        'deviceId': '5454545',
        'hardwareVendor': 'Trezor',
        'name': 'Trezor' },
      { 'address': '0x2F015C60E0be116B1f0CD534704Db9c92118FB6A',
        'derivationPath': 'm/44\'/60\'/0\'/1',
        'deviceId': '5454545',
        'hardwareVendor': 'Trezor',
        'name': 'Trezor' }])
})

test('Extract accounts from unknown device', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(true)
  return expect(hardwareKeyring.getAccounts(-2, 1, 'unknown'))
  .rejects.toThrow()
})
