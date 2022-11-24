/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import SolanaLedgerBridgeKeyring from './sol_ledger_bridge_keyring'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'
import { BraveWallet } from '../../../constants/types'
import { GetAccountsHardwareOperationResult, SignHardwareOperationResult, SolDerivationPaths } from '../types'
import { LedgerCommand, LedgerError, UnlockResponse } from './ledger-messages'
import {
  SolGetAccountResponse,
  SolGetAccountResponsePayload,
  SolSignTransactionResponse
} from './sol-ledger-messages'

// To use the MockLedgerTransport, we must overwrite
// the protected `transport` attribute, which yields a typescript
// error unless we use bracket notation, i.e. keyring['transport']
// instead of keyring.transport. As a result we silence the dot-notation
// tslint rule for the file.
//
/* eslint-disable @typescript-eslint/dot-notation */
const createKeyring = () => {
  let keyring = new SolanaLedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  keyring['transport'] = transport
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring['bridge'] = iframe

  return keyring
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

test('Check ledger bridge type', () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  return expect(keyring.type()).toStrictEqual(BraveWallet.LEDGER_HARDWARE_VENDOR)
})

test('getAccounts unlock error', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(-2, 1)
  const expectedResult: GetAccountsHardwareOperationResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('getAccounts success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)

  const getAccountsResponsePayload1: SolGetAccountResponsePayload = {
    success: true,
    address: Buffer.from('address for 44\'/501\'/0\'/0\'')
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload1 })
  const getAccountsResponsePayload2: SolGetAccountResponsePayload = {
    success: true,
    address: Buffer.from('address for 44\'/501\'/1\'/0\'')
  }
  keyring['transport']['addSendCommandResponse']({ payload: getAccountsResponsePayload2 })

  const result = await keyring.getAccounts(-2, 1, SolDerivationPaths.Default)
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
        coin: BraveWallet.CoinType.SOL,
        network: undefined
      },
      {
        address: '',
        addressBytes: Buffer.from('address for 44\'/501\'/1\'/0\''),
        derivationPath: "44'/501'/1'/0'",
        name: 'Ledger',
        hardwareVendor: 'Ledger',
        deviceId: '0d09bdd791abfcf562035fc99c7293400125339df1e8194b4ea8c2bd69327caa',
        coin: BraveWallet.CoinType.SOL,
        network: undefined
      }
    ]
  })
})

test('getAccounts ledger error after successful unlock', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
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

  keyring['transport']['addSendCommandResponse']({ payload: getAccountResponseLedgerError })
  const result: GetAccountsHardwareOperationResult = await keyring.getAccounts(-2, 1, SolDerivationPaths.LedgerLive)

  // TODO why is this different from the eth counterpart test
  expect(result).toEqual({
    success: false,
    error: {
      id: LedgerCommand.GetAccount,
      origin: window.origin,
      command: LedgerCommand.GetAccount,
      payload: { success: false, message: 'LedgerError', statusCode: 101 }
    },
    code: undefined
  })
})

test('signTransaction unlock error', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  const result = await keyring.signTransaction('44\'/501\'/1\'/0\'', Buffer.from('transaction'))
  const expectedResult: SignHardwareOperationResult = unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('signTransaction success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const signTransactionResponse: SolSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      signature: Buffer.from('signature')
    }
  }
  keyring['transport']['addSendCommandResponse'](signTransactionResponse)
  const result: SignHardwareOperationResult = await keyring.signTransaction(
    '44\'/501\'/1\'/0\'',
    Buffer.from('transaction')
  )

  const expectedResult: SignHardwareOperationResult = {
    success: true,
    payload: Buffer.from('signature')
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
  const signTransactionResponseLedgerError: SolSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: ledgerError
  }
  keyring['transport']['addSendCommandResponse'](signTransactionResponseLedgerError)
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
