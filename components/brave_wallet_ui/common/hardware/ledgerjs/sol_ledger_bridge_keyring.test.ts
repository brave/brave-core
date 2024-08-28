/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import SolanaLedgerBridgeKeyring from './sol_ledger_bridge_keyring'
import {
  AccountFromDevice,
  HardwareOperationResultAccounts,
  HardwareOperationError,
  HardwareOperationResultSolanaSignature,
  SolLedgerDefaultHardwareImportScheme
} from '../types'
import {
  LedgerCommand,
  UnlockResponse,
  SolGetAccountResponse,
  SolSignTransactionResponse,
  LedgerResponse
} from './ledger-messages'
import { MockLedgerTransport } from './mock_ledger_transport'

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
    error: 'LedgerError',
    code: 101
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
  const result: HardwareOperationResultAccounts = await keyring.getAccounts(
    0,
    1,
    SolLedgerDefaultHardwareImportScheme
  )

  const expectedResult: HardwareOperationError = {
    success: false,
    code: 101,
    error: 'LedgerError'
  }
  expect(result).toEqual(expectedResult)
})

test('getAccounts success', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)

  const getAccountsResponsePayload1: LedgerResponse<{
    address: Buffer
  }> = {
    success: true,
    address: Buffer.from("address for 44'/501'/0'/0'")
  }
  transport.addSendCommandResponse({
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: getAccountsResponsePayload1
  })
  const getAccountsResponsePayload2: LedgerResponse<{
    address: Buffer
  }> = {
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

  const expectedResult: AccountFromDevice[] = [
    {
      address: '3yyGpgRsxQWmrP8UZUjC87APcNdwPLuNEdLr',
      derivationPath: "44'/501'/0'/0'"
    },
    {
      address: '3yyGpgRsxQWmrP8UZUjC87APcNdwPM1umTV8',
      derivationPath: "44'/501'/1'/0'"
    }
  ]
  expect(result).toEqual({
    success: true,
    accounts: expectedResult
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
      error: 'LedgerError',
      code: 101
    }
  }

  transport.addSendCommandResponse(getAccountResponseLedgerError)
  const result: HardwareOperationResultAccounts = await keyring.getAccounts(
    0,
    1,
    SolLedgerDefaultHardwareImportScheme
  )

  const expectedResult: HardwareOperationError = {
    success: false,
    error: 'LedgerError',
    code: 101
  }

  expect(result).toEqual(expectedResult)
})

test('signTransaction unlock error', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockErrorResponse)
  const result = await keyring.signTransaction(
    "44'/501'/1'/0'",
    Buffer.from('transaction')
  )
  expect(result).toEqual({
    success: false,
    error: 'LedgerError',
    code: 101
  })
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
      untrustedSignatureBytes: Buffer.from('signature')
    }
  }
  transport.addSendCommandResponse(signTransactionResponse)
  const result = await keyring.signTransaction(
    "44'/501'/1'/0'",
    Buffer.from('transaction')
  )

  const expectedResult: HardwareOperationResultSolanaSignature = {
    success: true,
    signature: {
      bytes: [...Buffer.from('signature')]
    }
  }
  expect(result).toEqual(expectedResult)
})

test('signTransaction ledger error after successful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const ledgerError: LedgerResponse = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  const signTransactionResponseLedgerError: SolSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: ledgerError
  }
  transport.addSendCommandResponse(signTransactionResponseLedgerError)
  const result = await keyring.signTransaction(
    "44'/501'/1'/0'",
    Buffer.from('transaction')
  )

  const expectedResult: HardwareOperationError = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  expect(result).toEqual(expectedResult)
})
