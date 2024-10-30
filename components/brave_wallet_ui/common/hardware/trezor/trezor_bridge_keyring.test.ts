/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '../../../../common/locale'
import {
  kTrezorBridgeUrl,
  TrezorFrameResponse,
  TrezorCommand,
  TrezorFrameCommand,
  UnlockResponse,
  UnlockCommand,
  GetAccountsCommand,
  TrezorGetAccountsResponse,
  SignTransactionResponse,
  TrezorErrorsCodes,
  SignMessageResponse,
  SignTypedMessageResponse
} from './trezor-messages'
import TrezorBridgeKeyring from './trezor_bridge_keyring'
import { TrezorBridgeTransport } from './trezor-bridge-transport'
import { TrezorCommandHandler } from './trezor-command-handler'
import { getMockedTransactionInfo } from '../../constants/mocks'
import { Unsuccessful } from './trezor-connect-types'
import {
  EthTrezorDefaultHardwareImportScheme,
  HardwareOperationError,
  HardwareOperationResult
} from '../types'

const createTransport = (
  url: string,
  hardwareTransport: TrezorBridgeTransport | TrezorCommandHandler
) => {
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
      ;(value as Function)(
        {
          type: 'message',
          origin: url,
          data: data
        },
        url
      )
    }
  }
  return hardwareTransport
}

const createTrezorTransport = (
  unlock: HardwareOperationResult,
  accounts?: TrezorGetAccountsResponse,
  signedPayload?: SignTransactionResponse,
  signedMessagePayload?: SignMessageResponse
) => {
  let hardwareTransport = createTransport(
    kTrezorBridgeUrl,
    new TrezorBridgeTransport(kTrezorBridgeUrl)
  )
  hardwareTransport.contentWindow = {
    postMessage: (message: any, command: any) => {
      expect(command).toStrictEqual(kTrezorBridgeUrl)
      if (message.command === TrezorCommand.Unlock) {
        hardwareTransport.postResponse({
          id: message.id,
          command: TrezorCommand.Unlock,
          payload: unlock
        })
      }
      if (message.command === TrezorCommand.GetAccounts) {
        hardwareTransport.postResponse({
          id: message.id,
          command: TrezorCommand.GetAccounts,
          payload: accounts
        })
      }
      if (message.command === TrezorCommand.SignTransaction) {
        hardwareTransport.postResponse({
          id: message.id,
          command: TrezorCommand.SignTransaction,
          payload: signedPayload
        })
      }
      if (message.command === TrezorCommand.SignMessage) {
        hardwareTransport.postResponse({
          id: message.id,
          command: TrezorCommand.SignMessage,
          payload: signedMessagePayload
        })
      }
      if (message.command === TrezorCommand.SignTypedMessage) {
        hardwareTransport.postResponse({
          id: message.id,
          command: message.command,
          payload: signedMessagePayload
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
  let hardwareTransport = createTransport(
    kTrezorBridgeUrl,
    new TrezorCommandHandler()
  )
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
        ;(value as Function)(
          {
            type: 'message',
            origin: kTrezorBridgeUrl,
            data: data,
            source: handler.mockedWindow
          },
          kTrezorBridgeUrl
        )
      }
    }
  })
}

test('Wait for responses', () => {
  const transport = createTrezorTransport(false)
  transport.expectWindowMessageSubscribers(0)
  expect(
    transport.addCommandHandler('1', (data: any) => {
      expect(data.result).toStrictEqual(true)
    })
  ).toStrictEqual(true)
  // second handler with same id
  expect(
    transport.addCommandHandler('1', (data: any) => {
      expect(data.result).toStrictEqual(true)
    })
  ).toStrictEqual(false)
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
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  hardwareKeyring.unlocked = true
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(true)
})

const createTrezorKeyringWithTransport = (
  unlock: HardwareOperationResult,
  accounts?: TrezorGetAccountsResponse,
  signedPayload?: SignTransactionResponse,
  signedMessagePayload?: SignMessageResponse
) => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  const transport = createTrezorTransport(
    unlock,
    accounts,
    signedPayload,
    signedMessagePayload
  )
  hardwareKeyring.sendTrezorCommand = async (
    command: TrezorFrameCommand,
    listener: Function
  ) => {
    return transport.sendCommandToTrezorFrame(command, listener)
  }
  return hardwareKeyring
}

test('Unlock device success', () => {
  const expected = { success: true }
  const hardwareKeyring = createTrezorKeyringWithTransport(expected)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).resolves.toStrictEqual(expected)
})

test('Bridge not ready', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  let hardwareTransport = createTransport(
    kTrezorBridgeUrl,
    new TrezorBridgeTransport(kTrezorBridgeUrl)
  )
  hardwareKeyring.sendTrezorCommand = (
    command: TrezorFrameCommand,
    listener: Function
  ) => {
    return hardwareTransport.sendCommandToTrezorFrame(command, listener)
  }
  hardwareKeyring.sendTrezorCommand('command1', () => {}).then()
  const result = hardwareKeyring.sendTrezorCommand('command1', () => {})
  return expect(result).resolves.toStrictEqual(TrezorErrorsCodes.BridgeNotReady)
})

test('Device is busy', () => {
  const hardwareKeyring = new TrezorBridgeKeyring()
  let hardwareTransport = createTransport(
    kTrezorBridgeUrl,
    new TrezorBridgeTransport(kTrezorBridgeUrl)
  )
  hardwareTransport.contentWindow = {
    postMessage: () => {
      // This is intentional
    }
  }
  hardwareTransport.createBridge = async () => {
    return hardwareTransport
  }
  hardwareKeyring.sendTrezorCommand = (
    command: TrezorFrameCommand,
    listener: Function
  ) => {
    return hardwareTransport.sendCommandToTrezorFrame(command, listener)
  }
  hardwareKeyring.sendTrezorCommand('command1', () => {}).then()
  const result = hardwareKeyring.sendTrezorCommand('command1', () => {})
  return expect(result).resolves.toStrictEqual(
    TrezorErrorsCodes.CommandInProgress
  )
})

test('Unlock device fail', () => {
  const hardwareKeyring = createTrezorKeyringWithTransport({
    success: false,
    error: 'error',
    code: undefined
  })
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).resolves.toStrictEqual({
    success: false,
    error: getLocale('braveWalletUnlockError'),
    code: ''
  })
})

test('Unlock device fail with error', () => {
  const response = {
    success: false,
    payload: { error: 'test_error', code: 'test_code' }
  }
  const hardwareKeyring = createTrezorKeyringWithTransport(response)
  expect(hardwareKeyring.isUnlocked()).toStrictEqual(false)
  return expect(hardwareKeyring.unlock()).resolves.toStrictEqual({
    error: 'test_error',
    code: 'test_code',
    success: false
  })
})

test('Extract accounts from locked device', () => {
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: false, error: 'error', code: undefined },
    { success: true, payload: [] }
  )
  return expect(
    hardwareKeyring.getAccounts(0, 1, EthTrezorDefaultHardwareImportScheme)
  ).resolves.toStrictEqual({
    success: false,
    error: getLocale('braveWalletUnlockError'),
    code: ''
  })
})

test('Extracting accounts from unlocked device fail to access bridge', () => {
  const expectedError = getLocale('braveWalletCreateBridgeError')
  const expectedCode = 'test_code'
  const response = {
    success: false,
    payload: { error: expectedError, code: expectedCode }
  } as Unsuccessful
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    response
  )
  return expect(
    hardwareKeyring.getAccounts(0, 1, EthTrezorDefaultHardwareImportScheme)
  ).resolves.toStrictEqual({
    error: response.payload.error,
    success: response.success,
    code: response.payload.code
  })
})

test('Extract accounts from unlocked device returned success', () => {
  const accounts = [
    {
      publicKey:
        '3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9' +
        'e0dca11a001cf066d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1d',
      serializedPath: "m/44'/60'/0'/0/0",
      fingerprint: 5454545
    },
    {
      publicKey:
        '20d55983d1707ff6e9ce32d583ad0f7acb3b0beb601927c4ff05f780787f' +
        '377afe463d329f4535c82457f87df56d0a70e16a9442c6ff6d59b8f113d6073e9744',
      serializedPath: "m/44'/60'/0'/0/1",
      fingerprint: 5454545
    }
  ]
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    { success: true, payload: accounts }
  )

  return expect(
    hardwareKeyring.getAccounts(0, 2, EthTrezorDefaultHardwareImportScheme)
  ).resolves.toStrictEqual({
    accounts: [
      {
        'address': '0x2F015C60E0be116B1f0CD534704Db9c92118FB6A',
        'derivationPath': "m/44'/60'/0'/0/0"
      },
      {
        'address': '0x8e926dF9926746ba352F4d479Fb5DE47382e83bE',
        'derivationPath': "m/44'/60'/0'/0/1"
      }
    ],
    success: true
  })
})

test('Extracting accounts from unlocked device returned success without zero index', () => {
  const accounts = [
    {
      publicKey:
        '3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae' +
        '52ef9e0dca11a001cf066d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1d',
      serializedPath: "m/44'/60'/0'/0/0",
      fingerprint: 5454545
    },
    {
      publicKey:
        '20d55983d1707ff6e9ce32d583ad0f7acb3b0beb601927c4ff05f7' +
        '80787f377afe463d329f4535c82457f87df56d0a70e16a9442c6ff6d59b8f113d6073e9744',
      serializedPath: "m/44'/60'/0'/0/1",
      fingerprint: 5454545
    }
  ]
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    { success: true, payload: accounts }
  )

  return expect(
    hardwareKeyring.getAccounts(0, 2, EthTrezorDefaultHardwareImportScheme)
  ).resolves.toStrictEqual({
    accounts: [
      {
        'address': '0x2F015C60E0be116B1f0CD534704Db9c92118FB6A',
        'derivationPath': "m/44'/60'/0'/0/0"
      },
      {
        'address': '0x8e926dF9926746ba352F4d479Fb5DE47382e83bE',
        'derivationPath': "m/44'/60'/0'/0/1"
      }
    ],
    success: true
  })
})

test('Add unlock command handler', () => {
  const hardwareTransport = createCommandHandler()
  hardwareTransport.addCommandHandler(
    TrezorCommand.Unlock,
    (command: UnlockCommand): Promise<UnlockResponse> => {
      expect(command.command).toStrictEqual(TrezorCommand.Unlock)
      hardwareTransport._commands_called.push(TrezorCommand.Unlock)
      return Promise.resolve({ result: true })
    }
  )
  const response = handlerResponse(hardwareTransport)
  hardwareTransport.postCommand({
    command: TrezorCommand.GetAccounts,
    origin: kTrezorBridgeUrl
  })
  hardwareTransport.postCommand({
    command: TrezorCommand.Unlock,
    origin: kTrezorBridgeUrl
  })
  return expect(response).resolves.toStrictEqual([
    { result: true },
    [TrezorCommand.Unlock]
  ])
})

test('Add get accounts command handler', () => {
  const hardwareTransport = createCommandHandler()
  hardwareTransport.addCommandHandler(
    TrezorCommand.GetAccounts,
    (command: GetAccountsCommand): Promise<UnlockResponse> => {
      expect(command.command).toStrictEqual(TrezorCommand.GetAccounts)
      hardwareTransport._commands_called.push(TrezorCommand.GetAccounts)
      return Promise.resolve({ result: true })
    }
  )
  const response = handlerResponse(hardwareTransport)
  hardwareTransport.postCommand({
    command: TrezorCommand.Unlock,
    origin: kTrezorBridgeUrl
  })
  hardwareTransport.postCommand({
    command: TrezorCommand.GetAccounts,
    origin: kTrezorBridgeUrl
  })
  return expect(response).resolves.toStrictEqual([
    { result: true },
    [TrezorCommand.GetAccounts]
  ])
})

test('Add multiple commands handlers', () => {
  const hardwareTransport = createCommandHandler()
  hardwareTransport.addCommandHandler(
    TrezorCommand.Unlock,
    (command: UnlockCommand): Promise<UnlockResponse> => {
      expect(command.command).toStrictEqual(TrezorCommand.Unlock)
      hardwareTransport._commands_called.push(TrezorCommand.Unlock)
      return Promise.resolve({ result: true })
    }
  )

  hardwareTransport.addCommandHandler(
    TrezorCommand.GetAccounts,
    (command: GetAccountsCommand): Promise<UnlockResponse> => {
      expect(command.command).toStrictEqual(TrezorCommand.GetAccounts)
      hardwareTransport._commands_called.push(TrezorCommand.GetAccounts)
      return Promise.resolve({ result: true })
    }
  )
  const response = handlerResponse(hardwareTransport)
  hardwareTransport.postCommand({
    command: TrezorCommand.Unlock,
    origin: kTrezorBridgeUrl
  })
  hardwareTransport.postCommand({
    command: TrezorCommand.GetAccounts,
    origin: kTrezorBridgeUrl
  })
  hardwareTransport.postCommand({
    command: TrezorCommand.GetAccounts,
    origin: kTrezorBridgeUrl
  })
  hardwareTransport.postCommand({
    command: TrezorCommand.Unlock,
    origin: kTrezorBridgeUrl
  })
  return expect(response).resolves.toStrictEqual([
    { result: true },
    [
      TrezorCommand.Unlock,
      TrezorCommand.GetAccounts,
      TrezorCommand.GetAccounts,
      TrezorCommand.Unlock
    ]
  ])
})

test('Sign transaction from unlocked device', () => {
  const txInfo = getMockedTransactionInfo()
  const signed: SignTransactionResponse = {
    success: true,
    payload: {
      vBytes: Buffer.from([1]),
      rBytes: Buffer.from([2, 3]),
      sBytes: Buffer.from([4, 5, 6])
    }
  }
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    undefined,
    signed
  )

  const expectedResult = {
    success: true,
    signature: {
      vBytes: [1],
      rBytes: [2, 3],
      sBytes: [4, 5, 6]
    }
  }

  return expect(
    hardwareKeyring.signTransaction(
      "m/44'/60'/0'/0",
      txInfo.id,
      txInfo.txDataUnion.ethTxData1559,
      '0x539'
    )
  ).resolves.toStrictEqual(expectedResult)
})

test('Sign transaction failed from unlocked device', () => {
  const txInfo = getMockedTransactionInfo()
  const signed: SignTransactionResponse = {
    success: false,
    payload: {
      error: 'Permissions not granted',
      code: 'Method_PermissionsNotGranted'
    }
  }
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    undefined,
    signed
  )

  const expectedResult: HardwareOperationError = {
    error: signed.payload.error,
    code: signed.payload.code,
    success: false
  }

  return expect(
    hardwareKeyring.signTransaction(
      "m/44'/60'/0'/0",
      txInfo.id,
      txInfo.txDataUnion.ethTxData1559,
      '0x539'
    )
  ).resolves.toStrictEqual(expectedResult)
})

test('Sign message from unlocked device success', () => {
  const signMessagePayload: SignMessageResponse = {
    success: true,
    payload: {
      bytes: Buffer.from([3, 1, 4])
    }
  }
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    undefined,
    undefined,
    signMessagePayload
  )

  const expectedResult = {
    success: true,
    signature: {
      bytes: [3, 1, 4]
    }
  }

  return expect(
    hardwareKeyring.signPersonalMessage("m/44'/60'/0'/0", 'Hello!')
  ).resolves.toStrictEqual(expectedResult)
})

test('Sign message from unlocked device failed', () => {
  const signMessagePayload: Unsuccessful = {
    success: false,
    payload: {
      code: '1',
      error: 'error'
    }
  }
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    undefined,
    undefined,
    signMessagePayload
  )
  return expect(
    hardwareKeyring.signPersonalMessage("m/44'/60'/0'/0", 'Hello!')
  ).resolves.toStrictEqual({
    success: signMessagePayload.success,
    code: signMessagePayload.payload.code,
    error: signMessagePayload.payload.error
  })
})

test('Sign typed from unlocked device, success', () => {
  const signMessagePayload: SignTypedMessageResponse = {
    success: true,
    payload: {
      bytes: Buffer.from([2, 7, 8])
    }
  }
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    undefined,
    undefined,
    signMessagePayload
  )

  const expectedResult = {
    success: true,
    signature: {
      bytes: [2, 7, 8]
    }
  }

  return expect(
    hardwareKeyring.signEip712Message(
      "m/44'/60'/0'/0",
      'domainSeparatorHex',
      'hashStructMessageHex',
      '{}', // messageJson
      '{}', // domainJson
      '{}', // typesJson
      'primaryType'
    )
  ).resolves.toStrictEqual(expectedResult)
})

test('Sign typed message api not supported', () => {
  const signMessagePayload: SignTypedMessageResponse = {
    success: false,
    payload: {
      code: 'Method_InvalidParameter',
      error: 'some text'
    }
  }
  const hardwareKeyring = createTrezorKeyringWithTransport(
    { success: true },
    undefined,
    undefined,
    signMessagePayload
  )
  return expect(
    hardwareKeyring.signEip712Message(
      "m/44'/60'/0'/0",
      'domainSeparatorHex',
      'hashStructMessageHex',
      '{}', // messageJson
      '{}', // domainJson
      '{}', // typesJson
      'primaryType'
    )
  ).resolves.toStrictEqual({
    error: getLocale('braveWalletTrezorSignTypedDataError'),
    success: signMessagePayload.success,
    code: undefined
  })
})
