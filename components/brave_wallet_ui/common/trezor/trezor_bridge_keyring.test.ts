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
  TrezorError
} from '../../common/trezor/trezor-messages'
import {
  kTrezorHardwareVendor
} from '../../constants/types'
import { getLocale } from '../../../common/locale'

const createTrezorKeyring = (unlock: Boolean,
                             accounts?: TrezorGetAccountsResponse) => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.windowListeners_ = {}
  hardwareKeyring.getTrezorBridgeOrigin = () => {
    return 'braveWalletTrezorBridgeUrl'
  }
  hardwareKeyring.addWindowMessageListener = () => {
    hardwareKeyring.expectWindowMessageSubscribers(0)

    const key = hardwareKeyring.onMessageReceived.toString()
    hardwareKeyring.windowListeners_[key] =
        hardwareKeyring.onMessageReceived.bind(this)

    hardwareKeyring.expectWindowMessageSubscribers(1)
  }
  hardwareKeyring.expectWindowMessageSubscribers = (amount: number) => {
    const keys = Object.keys(hardwareKeyring.windowListeners_)
    expect(keys.length).toStrictEqual(amount)
  }
  hardwareKeyring.removeWindowMessageListener = () => {
    hardwareKeyring.expectWindowMessageSubscribers(1)
    const key = hardwareKeyring.onMessageReceived.toString()
    delete hardwareKeyring.windowListeners_[key]
    hardwareKeyring.expectWindowMessageSubscribers(0)
  }

  hardwareKeyring.postResponse = (data: TrezorFrameResponse) => {
    for (const value of Object.values(hardwareKeyring.windowListeners_)) {
      (value as Function)(
        { type: 'message',
          origin: kTrezorBridgeUrl,
          data: data
        }, kTrezorBridgeUrl)
    }
  }
  hardwareKeyring.contentWindow = {
    postMessage: (message: any, command: any) => {
      expect(command).toStrictEqual(kTrezorBridgeUrl)
      if (message.command === TrezorCommand.Unlock) {
        hardwareKeyring.postResponse({
          id: message.id,
          command: TrezorCommand.Unlock,
          result: unlock
        })
      }
      if (message.command === TrezorCommand.GetAccounts) {
        hardwareKeyring.postResponse({
          id: message.id,
          command: TrezorCommand.GetAccounts,
          payload: accounts
        })
      }
    }
  }
  hardwareKeyring._createBridge = async () => {
    return hardwareKeyring
  }

  return hardwareKeyring
}

test('Wait for responses', () => {
  const keyring = createTrezorKeyring(false)
  keyring.expectWindowMessageSubscribers(0)

  keyring.addEventListener(1, (data: any) => {
    expect(data.result).toStrictEqual(true)
  })
  keyring.addEventListener(2, (data: any) => {
    expect(data.result).toStrictEqual(true)
  })
  keyring.addEventListener(3, (data: any) => {
    expect(data.result).toStrictEqual(false)
  })

  keyring.expectWindowMessageSubscribers(1)

  // Unknown id
  keyring.postResponse({
    id: 4,
    command: TrezorCommand.Unlock,
    result: true
  })

  expect(Object.keys(keyring.pending_requests_).length).toStrictEqual(3)
  keyring.postResponse({
    id: 2,
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(Object.keys(keyring.pending_requests_).length).toStrictEqual(2)
  keyring.postResponse({
    id: 1,
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(Object.keys(keyring.pending_requests_).length).toStrictEqual(1)

  // same id twice
  keyring.postResponse({
    id: 1,
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(Object.keys(keyring.pending_requests_).length).toStrictEqual(1)

  keyring.postResponse({
    id: 3,
    command: TrezorCommand.Unlock,
    result: false
  })
  expect(keyring.pending_requests_).toStrictEqual(null)
  keyring.expectWindowMessageSubscribers(0)
})

test('isUnlocked', () => {
  const hardwareKeyring = createTrezorKeyring(true)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  hardwareKeyring.unlocked_ = true
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(true)
})

test('Unlock device success', () => {
  const hardwareKeyring = createTrezorKeyring(true)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).resolves.toStrictEqual(true)
})

test('Check trezor bridge type', () => {
  const hardwareKeyring = createTrezorKeyring(true,
                                              { success: true, payload: [] })
  return expect(hardwareKeyring.type()).toStrictEqual(kTrezorHardwareVendor)
})

test('Unlock device fail', () => {
  const hardwareKeyring = createTrezorKeyring(false)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).rejects.toStrictEqual(false)
})

test('Extract accounts from locked device', () => {
  const hardwareKeyring = createTrezorKeyring(false,
                                              { success: true, payload: [] })
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
  .rejects.toStrictEqual(false)
})

test('Extracting accounts from unlocked device fail to access bridge', () => {
  const error: TrezorError = { error: getLocale('braveWalletCreateBridgeError'),
    code: 'code' }
  const hardwareKeyring = createTrezorKeyring(true, { success: false, payload: error })
  const expected: TrezorBridgeAccountsPayload = {
    success: false, error: error.error, accounts: []
  }
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
    .rejects.toStrictEqual(expected)
})

test('Extracting accounts from unlocked device returned fail', () => {
  const error: TrezorError = { error: getLocale('braveWalletCreateBridgeError'),
    code: 'code' }
  const hardwareKeyring = createTrezorKeyring(true, { success: false, payload: error })
  hardwareKeyring._getBridge = () => {
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
  const hardwareKeyring = createTrezorKeyring(true, { success: true, payload: accounts })
  hardwareKeyring._getBridge = () => {
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
  const hardwareKeyring = createTrezorKeyring(true)
  return expect(hardwareKeyring.getAccounts(-2, 1, 'unknown'))
  .rejects.toThrow()
})
