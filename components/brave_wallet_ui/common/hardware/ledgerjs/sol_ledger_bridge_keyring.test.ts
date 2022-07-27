/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import SolanaLedgerBridgeKeyring from './sol_ledger_bridge_keyring'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'

let uuid = 0
window.crypto = {
  randomUUID () {
    return uuid++
  }
}

const createKeyring = () => {
  const keyring = new SolanaLedgerBridgeKeyring()
  const transport = new MockLedgerTransport()
  keyring.transport = transport
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring.bridge = iframe

  return keyring
}

test('getAccounts unlock error', async () => {
  const keyring = createKeyring()
  const sendCommandResponse: GetAccountsHardwareOperationResult = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(sendCommandResponse)
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(-2, 1)
  const expectedResult: GetAccountsHardwareOperationResult = sendCommandResponse.payload
  expect(result).toEqual(expectedResult)
})

test('getAccounts success', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)

  const getAccountsResponsePayload1: GetAccountsResponsePayload = {
    success: true,
    address: Buffer.from('address for 44\'/501\'/0\'/0\'')
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: GetAccountsResponsePayload = {
    success: true,
    address: Buffer.from('address for 44\'/501\'/1\'/0\'')
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload2 })

  const result = await keyring.getAccounts(-2, 1)
  expect(result).toEqual({
    success: true,
    payload: [
      {
        address: '',
        addressBytes: Buffer.from('address for 44\'/501\'/0\'/0\''),
        derivationPath: "44'/501'/0'/0'",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: '0d09bdd791abfcf562035fc99c7293400125339df1e8194b4ea8c2bd69327caa',
        coin: 501,
        network: undefined
      },
      {
        address: '',
        addressBytes: Buffer.from('address for 44\'/501\'/1\'/0\''),
        derivationPath: "44'/501'/1'/0'",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: '0d09bdd791abfcf562035fc99c7293400125339df1e8194b4ea8c2bd69327caa',
        coin: 501,
        network: undefined
      }
    ]
  })
})

test('getAccounts ledger error after successful unlock', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)

  const getAccountResponseLedgerError: GetAccountsResponsePayload = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }

  keyring.transport.addSendCommandResponse({ payload: getAccountResponseLedgerError })
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(-2, 1)

  expect(result).toEqual({
    success: false,
    error: { payload: { message: 'LedgerError', statusCode: 101 } },
    code: undefined
  })
})

test('signTransaction unlock error', async () => {
  const keyring = createKeyring()
  const ledgerErrorUnlockResponse: GetAccountsHardwareOperationResult = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(ledgerErrorUnlockResponse)
  const result = await keyring.signTransaction('44\'/501\'/1\'/0\'', Buffer.from('transaction'))
  const expectedResult: SignHardwareOperationResult = ledgerErrorUnlockResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signTransaction success', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)
  const signTransactionResponse: SignTransactionResponsePayload = {
    payload: {
      success: true,
      signature: Buffer.from('signature')
    }
  }
  keyring.transport.addSendCommandResponse(signTransactionResponse)
  const result: SignHardwareOperationResult = await keyring.signTransaction(
    '44\'/501\'/1\'/0\'',
    Buffer.from('transaction')
  )

  const expectedResult: SignHardwareOperationResult = {
    success: true,
    payload: signTransactionResponse.payload.signature
  }
  expect(result).toEqual(expectedResult)
})

test('signTransaction ledger error after successful unlock', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)
  const signTransactionResponseLedgerError: SignTransactionResponsePayload = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(signTransactionResponseLedgerError)
  const result: SignHardwareOperationResult = await keyring.signTransaction(
    '44\'/501\'/1\'/0\'',
    Buffer.from('transaction')
  )

  const expectedResult: SignHardwareOperationResult = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  expect(result).toEqual(expectedResult)
})
