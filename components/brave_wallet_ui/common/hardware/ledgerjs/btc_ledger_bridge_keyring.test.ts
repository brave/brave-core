/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import BitcoinLedgerBridgeKeyring from './btc_ledger_bridge_keyring'
import { MockLedgerTransport } from './ledger_bridge_keyring.test'
import {
  BtcLedgerMainnetHardwareImportScheme,
  GetAccountsHardwareOperationResult
} from '../types'
import {
  LedgerCommand,
  UnlockResponse,
  BtcGetAccountResponse
} from './ledger-messages'

const createKeyring = () => {
  let keyring = new BitcoinLedgerBridgeKeyring()
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
    BtcLedgerMainnetHardwareImportScheme
  )
  const expectedResult: GetAccountsHardwareOperationResult =
    unlockErrorResponse.payload
  expect(result).toEqual(expectedResult)
})

test('getAccounts success', async () => {
  const { keyring, transport } = createKeyring()
  transport.addSendCommandResponse(unlockSuccessResponse)

  const getAccountsResponse1: BtcGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      xpub: 'xpub1'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse1)

  const getAccountsResponse2: BtcGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: {
      success: true,
      xpub: 'xpub2'
    }
  }
  transport.addSendCommandResponse(getAccountsResponse2)

  const result = await keyring.getAccounts(
    0,
    2,
    BtcLedgerMainnetHardwareImportScheme
  )
  expect(result).toEqual({
    success: true,
    payload: [
      {
        address: 'xpub1',
        derivationPath: "84'/0'/0'"
      },
      {
        address: 'xpub2',
        derivationPath: "84'/0'/1'"
      }
    ]
  })
})

test('getAccounts ledger error after successful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const getAccountResponseLedgerError: BtcGetAccountResponse = {
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
    BtcLedgerMainnetHardwareImportScheme
  )

  expect(result).toEqual({
    code: 101,
    success: false,
    error: {
      success: false,
      message: 'LedgerError',
      statusCode: 101
    }
  })
})
