/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  FilLedgerMainnetHardwareImportScheme,
  FilLedgerTestnetHardwareImportScheme,
  SignHardwareOperationResult
} from '../types'
import FilecoinLedgerBridgeKeyring from './fil_ledger_bridge_keyring'
import {
  FilGetAccountResponse,
  FilLotusMessage,
  FilSignTransactionResponse,
  FilSignedLotusMessage,
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'

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
    message: 'LedgerError',
    statusCode: 101
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

  return expect(
    await keyring.getAccounts(0, 1, FilLedgerMainnetHardwareImportScheme)
  ).toEqual({
    payload: [
      {
        address: '0',
        derivationPath: "m/44'/461'/0'/0/0"
      }
    ],
    success: true
  })
})

test('Extracting accounts from device TEST', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  transport.addSendCommandResponse(getAccountsResponse)

  return expect(
    await keyring.getAccounts(0, 1, FilLedgerTestnetHardwareImportScheme)
  ).toEqual({
    payload: [
      {
        address: '0',
        derivationPath: "m/44'/1'/0'/0/0"
      }
    ],
    success: true
  })
})

test('Unlock device success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  expect(await keyring.unlock()).toEqual({ success: true })
})

test('Unlock device failed', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  expect(await keyring.unlock()).toEqual({
    message: 'LedgerError',
    statusCode: 101,
    success: false
  })
})

test('Extract accounts from locked device success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)

  return expect(
    await keyring.getAccounts(0, 2, FilLedgerTestnetHardwareImportScheme)
  ).toEqual({ message: 'LedgerError', statusCode: 101, success: false })
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
      lotusMessage: signedLotusMessage
    }
  }

  transport.addSendCommandResponse(signTransactionResponse)

  const result: SignHardwareOperationResult = await keyring.signTransaction(
    'transaction'
  )

  const expectedResult: SignHardwareOperationResult = {
    success: true,
    payload: signedLotusMessage
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
      lotusMessage: signedLotusMessage
    }
  }

  transport.addSendCommandResponse(signTransactionResponse)

  return expect(
    await keyring.signTransaction(JSON.stringify('message'))
  ).toEqual({ message: 'LedgerError', statusCode: 101, success: false })
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
      message: 'LedgerError',
      statusCode: 101
    }
  }
  transport.addSendCommandResponse(signTransactionResponse)

  return expect(await keyring.signTransaction('{,,')).toEqual({
    success: false,
    code: 101,
    error: {
      success: false,
      message: 'LedgerError',
      statusCode: 101
    }
  })
})
