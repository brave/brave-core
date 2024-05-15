/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { BraveWallet } from '../../../constants/types'
import { SignHardwareOperationResult } from '../types'
import {
  FilGetAccountResponse,
  FilLotusMessage,
  FilSignedLotusMessage,
  FilSignTransactionResponse
} from './fil-ledger-messages'
import FilecoinLedgerBridgeKeyring from './fil_ledger_bridge_keyring'
import { LedgerCommand, UnlockResponse } from './ledger-messages'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'

// To use the MockLedgerTransport, we must overwrite
// the protected `transport` attribute, which yields a typescript
// error unless we use bracket notation, i.e. keyring['transport']
// instead of keyring.transport. As a result we silent the dot-notation
// tslint rule for the file.
//
/* eslint-disable @typescript-eslint/dot-notation */

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
    accounts: ['0'],
    deviceId: 'device1'
  },
  command: LedgerCommand.GetAccount,
  id: LedgerCommand.GetAccount,
  origin: 'origin'
}

const createKeyring = (): FilecoinLedgerBridgeKeyring => {
  const ledgerHardwareKeyring = new FilecoinLedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  ledgerHardwareKeyring['transport'] = transport
  const iframe = document.createElement('iframe')
  iframe.setAttribute('sandbox', 'allow-scripts allow-same-origin')
  document.body.appendChild(iframe)
  ledgerHardwareKeyring['bridge'] = iframe
  return ledgerHardwareKeyring
}

test('Extracting accounts from device MAIN', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  keyring['transport']['addSendCommandResponse'](getAccountsResponse)

  return expect(
    await keyring.getAccounts(-2, 1, BraveWallet.FILECOIN_MAINNET)
  ).toEqual({
    payload: [
      {
        address: '0',
        coin: BraveWallet.CoinType.FIL,
        derivationPath: "m/44'/461'/0'/0/0",
        deviceId: 'device1',
        hardwareVendor: 'Ledger',
        name: 'Filecoin Ledger',
        keyringId: BraveWallet.KeyringId.kFilecoin
      }
    ],
    success: true
  })
})

test('Extracting accounts from device TEST', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  keyring['transport']['addSendCommandResponse'](getAccountsResponse)

  return expect(
    await keyring.getAccounts(-2, 1, BraveWallet.FILECOIN_TESTNET)
  ).toEqual({
    payload: [
      {
        address: '0',
        coin: BraveWallet.CoinType.FIL,
        derivationPath: "m/44'/1'/0'/0/0",
        deviceId: 'device1',
        hardwareVendor: 'Ledger',
        name: 'Filecoin Ledger',
        keyringId: BraveWallet.KeyringId.kFilecoinTestnet
      }
    ],
    success: true
  })
})

test('Check ledger bridge type', async () => {
  const ledgerHardwareKeyring = new FilecoinLedgerBridgeKeyring()
  return expect(ledgerHardwareKeyring.type()).toStrictEqual(
    LEDGER_HARDWARE_VENDOR
  )
})

test('Unlock device success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  expect(await keyring.unlock()).toEqual({ success: true })
})

test('Unlock device failed', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  expect(await keyring.unlock()).toEqual({
    message: 'LedgerError',
    statusCode: 101,
    success: false
  })
})

test('Extract accounts from locked device success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)

  return expect(
    await keyring.getAccounts(-2, 1, BraveWallet.FILECOIN_TESTNET)
  ).toEqual({ message: 'LedgerError', statusCode: 101, success: false })
})

test('signTransaction success', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
  const signTransactionResponse: FilSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      lotusMessage: signedLotusMessage
    }
  }

  keyring['transport']['addSendCommandResponse'](signTransactionResponse)

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
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockErrorResponse)
  const signTransactionResponse: FilSignTransactionResponse = {
    id: LedgerCommand.SignTransaction,
    origin: window.origin,
    command: LedgerCommand.SignTransaction,
    payload: {
      success: true,
      lotusMessage: signedLotusMessage
    }
  }

  keyring['transport']['addSendCommandResponse'](signTransactionResponse)

  return expect(
    await keyring.signTransaction(JSON.stringify('message'))
  ).toEqual({ message: 'LedgerError', statusCode: 101, success: false })
})

test('Sign transaction locked device, parsing error', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) {
    fail('transport should be defined')
  }
  keyring['transport']['addSendCommandResponse'](unlockSuccessResponse)
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
  keyring['transport']['addSendCommandResponse'](signTransactionResponse)

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
