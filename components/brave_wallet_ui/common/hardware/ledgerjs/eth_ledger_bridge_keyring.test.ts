/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import EthereumLedgerBridgeKeyring from './eth_ledger_bridge_keyring'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'
import { LedgerDerivationPaths } from '../types'
import { BraveWallet } from '../../../constants/types'

let uuid = 0
window.crypto = {
  randomUUID (): string  {
    return uuid++
  }
}

const createKeyring = () => {
  const keyring = new EthereumLedgerBridgeKeyring()
  const transport = new MockLedgerTransport()
  keyring.transport = transport
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring.bridge = iframe

  return keyring
}

test('Check ledger bridge type', () => {
  const keyring = createKeyring()
  return expect(keyring.type()).toStrictEqual(BraveWallet.LEDGER_HARDWARE_VENDOR)
})

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

test('getAccounts ledger live derivation path success', async () => {
  const keyring = createKeyring()
  const unlockSuccessResponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessResponse)

  const getAccountsResponsePayload1: EthGetAccountsResponsePayload = {
    success: true,
    publicKey: 'publicKey',
    address: 'address'
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: EthGetAccountsResponsePayload = {
    success: true,
    publicKey: 'publicKey 2',
    address: 'address 2'
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload2 })

  const result = await keyring.getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive)
  expect(result).toEqual({
    success: true,
    payload: [
      {
        address: 'address',
        derivationPath: "m/44'/60'/0'/0/0",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: 'd80c9bf910f144738ef983724bc04bd6bd3f17c5c83ed57bedee1b1b9278e811',
        coin: BraveWallet.CoinType.ETH,
        network: undefined
      },
      {
        address: 'address 2',
        derivationPath: "m/44'/60'/1'/0/0",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: 'd80c9bf910f144738ef983724bc04bd6bd3f17c5c83ed57bedee1b1b9278e811',
        coin: BraveWallet.CoinType.ETH,
        network: undefined
      }
    ]
  })
})

test('getAccounts legacy derivation path success', async () => {
  const keyring = createKeyring()
  const unlockSuccessResponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessResponse)

  const getAccountsResponsePayload1: EthGetAccountsResponsePayload = {
    success: true,
    publicKey: 'publicKey',
    address: 'address'
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: EthGetAccountsResponsePayload = {
    success: true,
    publicKey: 'publicKey 2',
    address: 'address 2'
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload2 })

  const result = await keyring.getAccounts(-2, 1, LedgerDerivationPaths.Legacy)
  expect(result).toEqual({
    success: true,
    payload: [
      {
        address: 'address',
        derivationPath: "m/44'/60'/0'/0",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: 'd80c9bf910f144738ef983724bc04bd6bd3f17c5c83ed57bedee1b1b9278e811',
        coin: BraveWallet.CoinType.ETH,
        network: undefined
      },
      {
        address: 'address 2',
        derivationPath: "m/44'/60'/0'/1",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: 'd80c9bf910f144738ef983724bc04bd6bd3f17c5c83ed57bedee1b1b9278e811',
        coin: BraveWallet.CoinType.ETH,
        network: undefined
      }
    ]
  })
})

test('getAccounts deprecated derivation path success', async () => {
  const keyring = createKeyring()
  const unlockSuccessResponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessResponse)

  const getAccountsResponsePayload1: EthGetAccountsResponsePayload = {
    success: true,
    publicKey: 'publicKey',
    address: 'address'
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: EthGetAccountsResponsePayload = {
    success: true,
    publicKey: 'publicKey 2',
    address: 'address 2'
  }
  keyring.transport.addSendCommandResponse({ payload: getAccountsResponsePayload2 })

  const result = await keyring.getAccounts(-2, 1, LedgerDerivationPaths.Deprecated)
  expect(result).toEqual({
    success: true,
    payload: [
      {
        address: 'address',
        derivationPath: "m/44'/60'/0'/0",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: 'd80c9bf910f144738ef983724bc04bd6bd3f17c5c83ed57bedee1b1b9278e811',
        coin: BraveWallet.CoinType.ETH,
        network: undefined
      },
      {
        address: 'address 2',
        derivationPath: "m/44'/60'/1'/0",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: 'd80c9bf910f144738ef983724bc04bd6bd3f17c5c83ed57bedee1b1b9278e811',
        coin: BraveWallet.CoinType.ETH,
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

  const getAccountResponseLedgerError: EthGetAccountsResponsePayload = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }

  keyring.transport.addSendCommandResponse({ payload: getAccountResponseLedgerError })
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive)

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
  const result = await keyring.signTransaction("m/44'/60'/0'/0/0", Buffer.from('transaction'))
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
      v: 'v',
      r: 'r',
      s: 's',
    }
  }
  keyring.transport.addSendCommandResponse(signTransactionResponse)
  const result: SignHardwareOperationResult = await keyring.signTransaction(
    '44\'/501\'/1\'/0\'',
    Buffer.from('transaction')
  )

  const expectedResult: SignHardwareOperationResult = {
    success: true,
    payload: {
      v: signTransactionResponse.payload.v,
      r: signTransactionResponse.payload.r,
      s: signTransactionResponse.payload.s,
    }
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
    "m/44'/60'/0'/0/0",
    'transaction'
  )

  const expectedResult: SignHardwareOperationResult = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage unlock error', async () => {
  const keyring = createKeyring()
  const ledgerErrorUnlockResponse: GetAccountsHardwareOperationResult = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(ledgerErrorUnlockResponse)
  const result = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )
  const expectedResult: SignHardwareOperationResult = ledgerErrorUnlockResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage success with padding v<27', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)
  const responsePayload: SignTransactionResponsePayload = {
    payload: {
      success: true,
      v: 0, r: 'b68983', s: 'r68983'
    }
  }
  keyring.transport.addSendCommandResponse(responsePayload)
  const result: SignHardwareOperationResult = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )

  const expectedResult = { payload: '0xb68983r6898300', success: true }
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage success with padding v>=27', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)
  const responsePayload: SignTransactionResponsePayload = {
    payload: {
      success: true,
      v: 28, r: 'b68983', s: 'r68983'
    }
  }
  keyring.transport.addSendCommandResponse(responsePayload)
  const result: SignHardwareOperationResult = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )

  const expectedResult = { payload: '0xb68983r6898301', success: true  }
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage failure after successful unlock', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)
  const signPersonalMessageResponse: SignTransactionResponsePayload = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(signPersonalMessageResponse)
  const result: SignHardwareOperationResult = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )
  const expectedResult: SignHardwareOperationResult = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  expect(result).toEqual(expectedResult)
})

test('signEip712Message unlock error', async () => {
  const keyring = createKeyring()
  const ledgerErrorUnlockResponse: GetAccountsHardwareOperationResult = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(ledgerErrorUnlockResponse)
  const result = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'message'
  )
  const expectedResult: SignHardwareOperationResult = ledgerErrorUnlockResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signEip712Message success', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)

  const responsePayload: SignTransactionResponsePayload = {
    payload: {
      success: true,
      v: 28, r: 'b68983', s: 'r68983'
    }
  }
  keyring.transport.addSendCommandResponse(responsePayload)
  const result: SignHardwareOperationResult = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )

  const expectedResult = { payload: '0xb68983r6898301', success: true  }
  expect(result).toEqual(expectedResult)
})

test('signEip712Message failure after successful unlock', async () => {
  const keyring = createKeyring()
  const unlockSuccessReponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(unlockSuccessReponse)
  const signPersonalMessageResponse: SignTransactionResponsePayload = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(signPersonalMessageResponse)
  const result: SignHardwareOperationResult = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'message'
  )
  const expectedResult: SignHardwareOperationResult = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  expect(result).toEqual(expectedResult)
})

