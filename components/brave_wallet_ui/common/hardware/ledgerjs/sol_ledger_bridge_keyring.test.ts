/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import SolanaLedgerBridgeKeyring from './sol_ledger_bridge_keyring'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'
import {
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult,
  SolLedgerDefaultHardwareImportScheme
} from '../types'
import {
  LedgerCommand,
  LedgerError,
  UnlockResponse,
  SolGetAccountResponse,
  SolGetAccountResponsePayload,
  SolSignTransactionResponse
} from './ledger-messages'

const createKeyring = () => {
  let keyring = new SolanaLedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  keyring.setTransportForTesting(transport)
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring.setBridgeForTesting(iframe)

  return { keyring, transport }
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

const unlockSuccessResponse: UnlockResponse = {
  id: LedgerCommand.Unlock,
  origin: window.origin,
  command: LedgerCommand.Unlock,
  payload: { success: true }
}

test('getAccounts unlock error', async () => {
  const { keyring, transport } = createKeyring()
  transport.addSendCommandResponse(unlockErrorResponse)
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(
    0,
    1,
    SolLedgerDefaultHardwareImportScheme
  )
  const expectedResult: GetAccountsHardwareOperationResult =
    unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('getAccounts success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)

  const getAccountsResponsePayload1: SolGetAccountResponsePayload = {
    success: true,
    address: Buffer.from("address for 44'/501'/0'/0'")
  }
  transport.addSendCommandResponse({
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: getAccountsResponsePayload1
  })
  const getAccountsResponsePayload2: SolGetAccountResponsePayload = {
    success: true,
    address: Buffer.from("address for 44'/501'/1'/0'")
  }
  transport.addSendCommandResponse({
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: getAccountsResponsePayload2
  })

  const result = await keyring.getAccounts(
    0,
    2,
    SolLedgerDefaultHardwareImportScheme
  )
  expect(result).toEqual({
    success: true,
    payload: [
      {
        address: '3yyGpgRsxQWmrP8UZUjC87APcNdwPLuNEdLr',
        derivationPath: "44'/501'/0'/0'"
      },
      {
        address: '3yyGpgRsxQWmrP8UZUjC87APcNdwPM1umTV8',
        derivationPath: "44'/501'/1'/0'"
      }
    ]
  })
})

test('getAccounts ledger error after successful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const getAccountResponseLedgerError: SolGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: false,
      message: 'LedgerError',
      statusCode: 101
    }
  }

  transport.addSendCommandResponse(getAccountResponseLedgerError)
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(
    0,
    1,
    SolLedgerDefaultHardwareImportScheme
  )

  // TODO why is this different from the eth counterpart test
  expect(result).toEqual({
    success: false,
    error: {
      success: false,
      message: 'LedgerError',
      statusCode: 101
    },
    code: 101
  })
})

test('signTransaction unlock error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  const result = await keyring.signTransaction(
    "44'/501'/1'/0'",
    Buffer.from('transaction')
  )
  const expectedResult: SignHardwareOperationResult =
    unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signTransaction success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const signTransactionResponse: SolSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      signature: Buffer.from('signature')
    }
  }
  transport.addSendCommandResponse(signTransactionResponse)
  const result: SignHardwareOperationResult = await keyring.signTransaction(
    "44'/501'/1'/0'",
    Buffer.from('transaction')
  )

  const expectedResult: SignHardwareOperationResult = {
    success: true,
    payload: Buffer.from('signature')
  }
  expect(result).toEqual(expectedResult)
})

test('signTransaction ledger error after successful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const ledgerError: LedgerError = {
    success: false,
    message: 'LedgerError',
    statusCode: 101
  }
  const signTransactionResponseLedgerError: SolSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: ledgerError
  }
  transport.addSendCommandResponse(signTransactionResponseLedgerError)
  const result: SignHardwareOperationResult = await keyring.signTransaction(
    "44'/501'/1'/0'",
    Buffer.from('transaction')
  )

  const expectedResult: SignHardwareOperationResult = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  expect(result).toEqual(expectedResult)
})
