/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import EthereumLedgerBridgeKeyring from './eth_ledger_bridge_keyring'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'
import { LedgerDerivationPaths, GetAccountsHardwareOperationResult, SignHardwareOperationResult } from '../types'
import { BraveWallet } from '../../../constants/types'
import { LedgerCommand, LedgerError, UnlockResponse } from './ledger-messages'
import {
  EthGetAccountResponsePayload,
  EthSignTransactionResponse,
  EthereumSignedTx,
  EthSignPersonalMessageResponse,
  EthSignEip712MessageResponse
} from './eth-ledger-messages'

// To use the MockLedgerTransport, we must overwrite
// the protected `transport` attribute, which yields a typescript
// error unless we use bracket notation, i.e. keyring['transport']
// instead of keyring.transport. As a result we silence the dot-notation
// tslint rule for the file.
//
/* eslint-disable @typescript-eslint/dot-notation */
const createKeyring = (): EthereumLedgerBridgeKeyring => {
  let keyring = new EthereumLedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  keyring['transport'] = transport
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring['bridge'] = iframe
  return keyring
}
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

test('Check ledger bridge type', () => {
  const keyring = createKeyring()
  return expect(keyring.type()).toStrictEqual(BraveWallet.LEDGER_HARDWARE_VENDOR)
})

test('getAccounts unlock error', async () => {
  const keyring: EthereumLedgerBridgeKeyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive)
  const expectedResult: GetAccountsHardwareOperationResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('getAccounts ledger live derivation path success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const getAccountsResponsePayload1: EthGetAccountResponsePayload = {
    success: true,
    publicKey: 'publicKey',
    address: 'address'
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: EthGetAccountResponsePayload = {
    success: true,
    publicKey: 'publicKey 2',
    address: 'address 2'
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload2 })

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
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const getAccountsResponsePayload1: EthGetAccountResponsePayload = {
    success: true,
    publicKey: 'publicKey',
    address: 'address'
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: EthGetAccountResponsePayload = {
    success: true,
    publicKey: 'publicKey 2',
    address: 'address 2'
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload2 })

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
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)

  const getAccountsResponsePayload1: EthGetAccountResponsePayload = {
    success: true,
    publicKey: 'publicKey',
    address: 'address'
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: EthGetAccountResponsePayload = {
    success: true,
    publicKey: 'publicKey 2',
    address: 'address 2'
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload2 })

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
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const getAccountResponseLedgerError: LedgerError = {
    success: false,
    message: 'LedgerError',
    statusCode: 101
  }

  keyring['transport']['addSendCommandResponse']({ payload: getAccountResponseLedgerError })
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(-2, 1, LedgerDerivationPaths.LedgerLive)

  expect(result).toEqual({
    success: false,
    error: { message: 'LedgerError', statusCode: 101, success: false },
    code: 101
  })
})

test('signTransaction unlock error', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  const result = await keyring.signTransaction("m/44'/60'/0'/0/0", 'transaction')
  const expectedResult: SignHardwareOperationResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signTransaction success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const signTransactionResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      v: 'v',
      r: 'r',
      s: 's'
    }
  }

  keyring['transport']['addSendCommandResponse'](signTransactionResponse)
  const result: SignHardwareOperationResult = await keyring.signTransaction(
    '44\'/501\'/1\'/0\'', 'transaction'
  )

  const expectedResult: SignHardwareOperationResult = {
    success: true,
    payload: {
      v: 'v',
      r: 'r',
      s: 's'
    } as EthereumSignedTx
  }
  expect(result).toEqual(expectedResult)
})

test('signTransaction ledger error after successful unlock', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const ledgerError: LedgerError = {
    success: false,
    message: 'LedgerError',
    statusCode: 101
  }

  const signTransactionResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: ledgerError
  }
  keyring['transport']['addSendCommandResponse'](signTransactionResponse)
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
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  const result = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )
  const expectedResult: SignHardwareOperationResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage success with padding v<27', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const responsePayload: EthSignPersonalMessageResponse = {
    id: LedgerCommand.SignPersonalMessage,
    origin: window.origin,
    command: LedgerCommand.SignPersonalMessage,
    payload: { success: true, v: 0, r: 'b68983', s: 'r68983' }
  }
  keyring['transport']['addSendCommandResponse'](responsePayload)
  const result: SignHardwareOperationResult = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )

  const expectedResult = { payload: '0xb68983r6898300', success: true }
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage success with padding v>=27', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const responsePayload: EthSignPersonalMessageResponse = {
    id: LedgerCommand.SignPersonalMessage,
    origin: window.origin,
    command: LedgerCommand.SignPersonalMessage,
    payload: { success: true, v: 28, r: 'b68983', s: 'r68983' }
  }
  keyring['transport']['addSendCommandResponse'](responsePayload)
  const result: SignHardwareOperationResult = await keyring.signPersonalMessage(
    "m/44'/60'/0'/0/0",
    'message'
  )

  const expectedResult = { payload: '0xb68983r6898301', success: true }
  expect(result).toEqual(expectedResult)
})

test('signPersonalMessage failure after successful unlock', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const ledgerError: LedgerError = {
    success: false,
    message: 'LedgerError',
    statusCode: 101
  }

  const signPersonalMessageResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: ledgerError
  }
  keyring['transport']['addSendCommandResponse'](signPersonalMessageResponse)
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
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  const result = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'domainSeparatorHex',
    'hashStructMessageHex'
  )
  const expectedResult: SignHardwareOperationResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signEip712Message success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const responsePayload: EthSignEip712MessageResponse = {
    id: LedgerCommand.SignEip712Message,
    origin: window.origin,
    command: LedgerCommand.SignEip712Message,
    payload: { success: true, v: 28, r: 'b68983', s: 'r68983' }
  }
  keyring['transport']['addSendCommandResponse'](responsePayload)
  const result: SignHardwareOperationResult = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'domainSeparatorHex',
    'hashStructMessageHex'
  )

  const expectedResult = { payload: '0xb68983r6898301', success: true }
  expect(result).toEqual(expectedResult)
})

test('signEip712Message failure after successful unlock', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const ledgerError: LedgerError = {
    success: false,
    message: 'LedgerError',
    statusCode: 101
  }
  const signEip712MessageResponse: EthSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: ledgerError
  }
  keyring['transport']['addSendCommandResponse'](signEip712MessageResponse)
  const result: SignHardwareOperationResult = await keyring.signEip712Message(
    "m/44'/60'/0'/0/0",
    'domainSeparatorHex',
    'hashStructMessageHex'
  )
  const expectedResult: SignHardwareOperationResult = {
    success: false,
    error: 'LedgerError',
    code: 101
  }
  expect(result).toEqual(expectedResult)
})
