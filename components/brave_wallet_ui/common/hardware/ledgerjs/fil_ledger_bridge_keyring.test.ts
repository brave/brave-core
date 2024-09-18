/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  FilLedgerMainnetHardwareImportScheme,
  FilLedgerTestnetHardwareImportScheme,
  HardwareOperationResultAccounts,
  HardwareOperationError,
  HardwareOperationResultFilecoinSignature
} from '../types'
import FilecoinLedgerBridgeKeyring from './fil_ledger_bridge_keyring'
import {
  FilGetAccountResponse,
  FilSignTransactionResponse,
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import { MockLedgerTransport } from './mock_ledger_transport'

const unlockSuccessResponse: UnlockResponse = {
  id: LedgerCommand.Unlock,
  origin: window.origin,
  command: LedgerCommand.Unlock,
  payload: { success: true }
}

const unlockErrorResponse: UnlockResponse = {
  id: LedgerCommand.Unlock,
  origin: window.origin,
  command: LedgerCommand.Unlock,
  payload: {
    success: false,
    error: 'LedgerError',
    code: 101
  }
}

interface FilLotusMessage {
  To: string
  From: string
  Nonce: number
  Value: string
  GasPremium: string
  GasLimit: number
  GasFeeCap: string
  Method: number
  Params?: string | string[]
}

interface FilSignedLotusMessage {
  Message: FilLotusMessage
  Signature: {
    Type: number
    Data: string
  }
}

const lotusMessage: FilLotusMessage = {
  To: 'to',
  From: 'from',
  Nonce: 0,
  Value: 'value',
  GasPremium: 'gas',
  GasLimit: 1,
  GasFeeCap: 'cap',
  Method: 1,
  Params: 'params'
}

const signedLotusMessage: FilSignedLotusMessage = {
  Message: lotusMessage,
  Signature: {
    Type: 1,
    Data: 'data'
  }
}

const signedLotusMessageJSON = JSON.stringify(signedLotusMessage)

const getAccountsResponse: FilGetAccountResponse = {
  payload: {
    success: true,
    accounts: ['0']
  },
  command: LedgerCommand.GetAccount,
  id: LedgerCommand.GetAccount,
  origin: 'origin'
}

const createKeyring = () => {
  const keyring = new FilecoinLedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  keyring.setTransportForTesting(transport)
  const iframe = document.createElement('iframe')
  iframe.setAttribute('sandbox', 'allow-scripts allow-same-origin')
  document.body.appendChild(iframe)
  keyring.setBridgeForTesting(iframe)
  return { keyring, transport }
}

test('Extracting accounts from device MAIN', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  transport.addSendCommandResponse(getAccountsResponse)

  const expectedResult: HardwareOperationResultAccounts = {
    success: true,
    accounts: [
      {
        address: '0',
        derivationPath: "m/44'/461'/0'/0/0"
      }
    ]
  }

  return expect(
    await keyring.getAccounts(0, 1, FilLedgerMainnetHardwareImportScheme)
  ).toEqual(expectedResult)
})

test('Extracting accounts from device TEST', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  transport.addSendCommandResponse(getAccountsResponse)

  const expectedResult: HardwareOperationResultAccounts = {
    success: true,
    accounts: [
      {
        address: '0',
        derivationPath: "m/44'/1'/0'/0/0"
      }
    ]
  }

  return expect(
    await keyring.getAccounts(0, 1, FilLedgerTestnetHardwareImportScheme)
  ).toEqual(expectedResult)
})

test('Unlock device success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  expect(await keyring.unlock()).toEqual({ success: true })
})

test('Unlock device failed', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)

  const expectedResult: HardwareOperationError = {
    success: false,
    error: 'LedgerError',
    code: 101
  }

  expect(await keyring.unlock()).toEqual(expectedResult)
})

test('Extract accounts from locked device success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)

  const expectedResult: HardwareOperationError = {
    success: false,
    error: 'LedgerError',
    code: 101
  }

  return expect(
    await keyring.getAccounts(0, 2, FilLedgerTestnetHardwareImportScheme)
  ).toEqual(expectedResult)
})

test('signTransaction success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const signTransactionResponse: FilSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      untrustedSignedTxJson: signedLotusMessageJSON
    }
  }

  transport.addSendCommandResponse(signTransactionResponse)

  const result = await keyring.signTransaction('transaction')

  const expectedResult: HardwareOperationResultFilecoinSignature = {
    success: true,
    signature: {
      signedMessageJson: signedLotusMessageJSON
    }
  }
  expect(result).toEqual(expectedResult)
})

test('Sign transaction locked device, unlock error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  const signTransactionResponse: FilSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      untrustedSignedTxJson: signedLotusMessageJSON
    }
  }

  transport.addSendCommandResponse(signTransactionResponse)

  const expectedResult: HardwareOperationError = {
    success: false,
    error: 'LedgerError',
    code: 101
  }

  return expect(
    await keyring.signTransaction(JSON.stringify('message'))
  ).toEqual(expectedResult)
})

test('Sign transaction locked device, parsing error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const signTransactionResponse: FilSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: false,
      error: 'LedgerError',
      code: 101
    }
  }
  transport.addSendCommandResponse(signTransactionResponse)

  const expectedResult: HardwareOperationError = {
    success: false,
    error: 'LedgerError',
    code: 101
  }

  return expect(await keyring.signTransaction('{,,')).toEqual(expectedResult)
})
