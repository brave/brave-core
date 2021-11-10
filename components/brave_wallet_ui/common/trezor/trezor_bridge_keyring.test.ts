/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import TrezorBridgeKeyring from './trezor_bridge_keyring'
import {
  TrezorDerivationPaths
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import {
  kTrezorBridgeUrl,
  TrezorGetPublicKeyResponse,
  TrezorFrameResponse,
  TrezorCommand,
  TrezorError,
  TrezorFrameCommand,
  UnlockResponse,
  UnlockCommand,
  GetAccountsCommand
} from '../../common/trezor/trezor-messages'
import {
  TREZOR_HARDWARE_VENDOR
} from '../../constants/types'
import { getLocale } from '../../../common/locale'
import { TrezorBridgeTransport } from './trezor-bridge-transport'
import { TrezorCommandHandler } from './trezor-command-handler'

let uuid = 0
window.crypto = {
  randomUUID () {
    return uuid++
  }
}

const createTrezorTransport = (unlock: Boolean,
                               accounts?: TrezorGetPublicKeyResponse) => {
  const hardwareTransport = new TrezorBridgeTransport(kTrezorBridgeUrl)
  hardwareTransport.windowListeners_ = {}
  hardwareTransport.getTrezorBridgeOrigin = () => {
    return 'braveWalletTrezorBridgeUrl'
  }
  hardwareTransport.addWindowMessageListener = () => {
    hardwareTransport.expectWindowMessageSubscribers(0)

    const key = hardwareTransport.onMessageReceived.toString()
    hardwareTransport.windowListeners_[key] = hardwareTransport.onMessageReceived

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

const createCommandHandler = () => {
  const hardwareTransport = new TrezorCommandHandler()
  hardwareTransport.windowListeners_ = {}
  hardwareTransport.getTrezorBridgeOrigin = () => {
    return 'braveWalletTrezorBridgeUrl'
  }
  hardwareTransport.addWindowMessageListener = () => {
    hardwareTransport.expectWindowMessageSubscribers(0)

    const key = hardwareTransport.onMessageReceived.toString()
    hardwareTransport.windowListeners_[key] = hardwareTransport.onMessageReceived

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
  hardwareTransport._commands_called = []
  return hardwareTransport
}

const handlerResponse = async (handler: TrezorCommandHandler): Promise => {
  return new Promise(async (resolve) => {
    handler.mockedWindow = {
      postMessage: (message: any, command: any) => {
        return resolve([message, handler._commands_called])
      }
    }

    handler.postCommand = (data: TrezorFrameCommand) => {
      for (const value of Object.values(handler.windowListeners_)) {
        (value as Function)(
          { type: 'message',
            origin: kTrezorBridgeUrl,
            data: data,
            source: handler.mockedWindow
          }, kTrezorBridgeUrl)
      }
    }
  })
}

test('Wait for responses', () => {
  const transport = createTrezorTransport(false)
  transport.expectWindowMessageSubscribers(0)
  transport.addCommandHandler('1', (data: any) => {
    expect(data.result).toStrictEqual(true)
  })
  transport.addCommandHandler('2', (data: any) => {
    expect(data.result).toStrictEqual(true)
  })
  transport.addCommandHandler('3', (data: any) => {
    expect(data.result).toStrictEqual(false)
  })

  transport.expectWindowMessageSubscribers(1)

  // Unknown id
  transport.postResponse({
    id: '4',
    command: TrezorCommand.Unlock,
    result: true
  })

  expect(transport.handlers.size).toStrictEqual(3)
  transport.postResponse({
    id: '2',
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(transport.handlers.size).toStrictEqual(2)
  transport.postResponse({
    id: '1',
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(transport.handlers.size).toStrictEqual(1)

  // same id twice
  transport.postResponse({
    id: '1',
    command: TrezorCommand.Unlock,
    result: true
  })
  expect(transport.handlers.size).toStrictEqual(1)

  transport.postResponse({
    id: '3',
    command: TrezorCommand.Unlock,
    result: false
  })
  expect(transport.handlers.size).toStrictEqual(0)
  transport.expectWindowMessageSubscribers(0)
})

test('isUnlocked', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(false)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  hardwareKeyring.unlocked_ = true
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(true)
})

const createTrezorKeyringWithTransport = (unlock: Boolean,
                                          accounts?: TrezorGetAccountsResponse) => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  const transport = createTrezorTransport(unlock, accounts)
  hardwareKeyring.sendTrezorCommand = async (command: TrezorFrameCommand, listener: Function) => {
    return transport.sendCommandToTrezorFrame(command, listener)
  }
  return hardwareKeyring
}

test('Unlock device success', () => {
  const hardwareKeyring = createTrezorKeyringWithTransport(true)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).resolves.toStrictEqual(true)
})

test('Check trezor bridge type', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  hardwareKeyring.transport_ = createTrezorTransport(true,
                                              { success: true, payload: [], id: 1 })
  return expect(hardwareKeyring.type()).toStrictEqual(TREZOR_HARDWARE_VENDOR)
})

test('Unlock device fail', () => {
  const hardwareKeyring = createTrezorKeyringWithTransport(false)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).resolves.toStrictEqual(false)
})

test('Extract accounts from locked device', () => {
  const hardwareKeyring = createTrezorKeyringWithTransport(
    false, { success: true, payload: [] })
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
  .resolves.toStrictEqual(new Error(getLocale('braveWalletUnlockError')))
})

test('Extracting accounts from unlocked device fail to access bridge', () => {
  const error: TrezorError = { error: getLocale('braveWalletCreateBridgeError'),
    code: 'code' }
  const hardwareKeyring = createTrezorKeyringWithTransport(true, { success: false, payload: error })
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
    .rejects.toStrictEqual(new Error(getLocale('braveWalletCreateBridgeError')))
})

test('Extracting accounts from unlocked device returned fail', () => {
  const error: TrezorError = { error: getLocale('braveWalletCreateBridgeError'),
    code: 'code' }
  const hardwareKeyring = createTrezorKeyringWithTransport(true, { success: false, payload: error })
  hardwareKeyring.getBridge = () => {
    return hardwareKeyring as any
  }
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
    .rejects.toStrictEqual(new Error(getLocale('braveWalletCreateBridgeError')))
})

test('Extracting accounts from unlocked device returned success', () => {
  const accounts = [
    {
      publicKey: '3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9e0dca11a001cf066d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1d',
      serializedPath: 'm/44\'/60\'/0\'/0',
      fingerprint: 5454545
    },
    {
      publicKey: '20d55983d1707ff6e9ce32d583ad0f7acb3b0beb601927c4ff05f780787f377afe463d329f4535c82457f87df56d0a70e16a9442c6ff6d59b8f113d6073e9744',
      serializedPath: 'm/44\'/60\'/0\'/1',
      fingerprint: 5454545
    }
  ]
  const hardwareKeyring = createTrezorKeyringWithTransport(true, { success: true, payload: accounts })
  hardwareKeyring.getBridge = () => {
    return hardwareKeyring as any
  }
  hardwareKeyring.getHashFromAddress = async (address: string) => {
    return address === '0x2F015C60E0be116B1f0CD534704Db9c92118FB6A' ? '5454545' : '111'
  }
  return expect(hardwareKeyring.getAccounts(-2, 1, TrezorDerivationPaths.Default))
    .resolves.toStrictEqual([
      { 'address': '0x2F015C60E0be116B1f0CD534704Db9c92118FB6A',
        'derivationPath': 'm/44\'/60\'/0\'/0',
        'deviceId': '5454545',
        'hardwareVendor': 'Trezor',
        'name': 'Trezor' },
      { 'address': '0x8e926dF9926746ba352F4d479Fb5DE47382e83bE',
        'derivationPath': 'm/44\'/60\'/0\'/1',
        'deviceId': '5454545',
        'hardwareVendor': 'Trezor',
        'name': 'Trezor' }])
})

test('Extracting accounts from unlocked device returned success without zero index', () => {
  const accounts = [
    {
      publicKey: '3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9e0dca11a001cf066d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1d',
      serializedPath: 'm/44\'/60\'/0\'/0',
      fingerprint: 5454545
    },
    {
      publicKey: '20d55983d1707ff6e9ce32d583ad0f7acb3b0beb601927c4ff05f780787f377afe463d329f4535c82457f87df56d0a70e16a9442c6ff6d59b8f113d6073e9744',
      serializedPath: 'm/44\'/60\'/0\'/1',
      fingerprint: 5454545
    }
  ]
  const hardwareKeyring = createTrezorKeyringWithTransport(true, { success: true, payload: accounts })
  hardwareKeyring.getBridge = () => {
    return hardwareKeyring as any
  }
  hardwareKeyring.getHashFromAddress = async (address: string) => {
    return address === '0x2F015C60E0be116B1f0CD534704Db9c92118FB6A' ? '5454545' : '111'
  }
  return expect(hardwareKeyring.getAccounts(1, 2, TrezorDerivationPaths.Default))
    .resolves.toStrictEqual([
      { 'address': '0x8e926dF9926746ba352F4d479Fb5DE47382e83bE',
        'derivationPath': 'm/44\'/60\'/0\'/1',
        'deviceId': '5454545',
        'hardwareVendor': 'Trezor',
        'name': 'Trezor' }])
})

test('Extract accounts from unknown device', () => {
  const hardwareKeyring = createTrezorKeyringWithTransport(true)
  return expect(hardwareKeyring.getAccounts(-2, 1, 'unknown'))
  .rejects.toThrow()
})

test('Add unlock command handler', () => {
  const hardwareTransport = createCommandHandler()
  hardwareTransport.addCommandHandler(TrezorCommand.Unlock,
      (command: UnlockCommand): Promise<UnlockResponse> => {
        expect(command.command).toStrictEqual(TrezorCommand.Unlock)
        hardwareTransport._commands_called.push(TrezorCommand.Unlock)
        return Promise.resolve({ result: true })
      })
  const response = handlerResponse(hardwareTransport)
  hardwareTransport.postCommand({ command: TrezorCommand.GetAccounts, origin: kTrezorBridgeUrl })
  hardwareTransport.postCommand({ command: TrezorCommand.Unlock, origin: kTrezorBridgeUrl })
  return expect(response).resolves.toStrictEqual([{ result: true }, [TrezorCommand.Unlock]])
})

test('Add get accounts command handler', () => {
  const hardwareTransport = createCommandHandler()
  hardwareTransport.addCommandHandler(TrezorCommand.GetAccounts,
      (command: GetAccountsCommand): Promise<UnlockResponse> => {
        expect(command.command).toStrictEqual(TrezorCommand.GetAccounts)
        hardwareTransport._commands_called.push(TrezorCommand.GetAccounts)
        return Promise.resolve({ result: true })
      })
  const response = handlerResponse(hardwareTransport)
  hardwareTransport.postCommand({ command: TrezorCommand.Unlock, origin: kTrezorBridgeUrl })
  hardwareTransport.postCommand({ command: TrezorCommand.GetAccounts, origin: kTrezorBridgeUrl })
  return expect(response).resolves.toStrictEqual([{ result: true }, [TrezorCommand.GetAccounts]])
})

test('Add multiple commands handlers', () => {
  const hardwareTransport = createCommandHandler()
  hardwareTransport.addCommandHandler(TrezorCommand.Unlock,
    (command: UnlockCommand): Promise<UnlockResponse> => {
      expect(command.command).toStrictEqual(TrezorCommand.Unlock)
      hardwareTransport._commands_called.push(TrezorCommand.Unlock)
      return Promise.resolve({ result: true })
    })

  hardwareTransport.addCommandHandler(TrezorCommand.GetAccounts,
      (command: GetAccountsCommand): Promise<UnlockResponse> => {
        expect(command.command).toStrictEqual(TrezorCommand.GetAccounts)
        hardwareTransport._commands_called.push(TrezorCommand.GetAccounts)
        return Promise.resolve({ result: true })
      })
  const response = handlerResponse(hardwareTransport)
  hardwareTransport.postCommand({ command: TrezorCommand.Unlock, origin: kTrezorBridgeUrl })
  hardwareTransport.postCommand({ command: TrezorCommand.GetAccounts, origin: kTrezorBridgeUrl })
  hardwareTransport.postCommand({ command: TrezorCommand.GetAccounts, origin: kTrezorBridgeUrl })
  hardwareTransport.postCommand({ command: TrezorCommand.Unlock, origin: kTrezorBridgeUrl })
  return expect(response).resolves.toStrictEqual([
    { result: true },
    [ TrezorCommand.Unlock, TrezorCommand.GetAccounts,
      TrezorCommand.GetAccounts, TrezorCommand.Unlock]])
})
