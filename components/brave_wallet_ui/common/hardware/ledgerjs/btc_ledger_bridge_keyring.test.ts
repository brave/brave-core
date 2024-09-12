/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import BitcoinLedgerBridgeKeyring from './btc_ledger_bridge_keyring'
import { MockLedgerTransport } from './mock_ledger_transport'
import {
  BtcLedgerMainnetHardwareImportScheme,
  HardwareOperationError,
  HardwareOperationResultAccounts
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

const unlockError: HardwareOperationError = {
  success: false,
  error: 'LedgerError',
  code: 101
}

const unlockErrorResponse: UnlockResponse = {
  id: LedgerCommand.Unlock,
  origin: window.origin,
  command: LedgerCommand.Unlock,
  payload: unlockError
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
  const result = await keyring.getAccounts(
    0,
    1,
    BtcLedgerMainnetHardwareImportScheme
  )
  const expectedResult: HardwareOperationError = unlockError
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
  const expectedResult: HardwareOperationResultAccounts = {
    success: true,
    accounts: [
      {
        address: 'xpub1',
        derivationPath: "84'/0'/0'"
      },
      {
        address: 'xpub2',
        derivationPath: "84'/0'/1'"
      }
    ]
  }
  expect(result).toEqual(expectedResult)
})

test('getAccounts ledger error after successful unlock', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(unlockSuccessResponse)
  const getAccountResponseLedgerError: BtcGetAccountResponse = {
    id: LedgerCommand.GetAccount,
    origin: window.origin,
    command: LedgerCommand.GetAccount,
    payload: unlockError
  }

  transport.addSendCommandResponse(getAccountResponseLedgerError)
  const result = await keyring.getAccounts(
    0,
    1,
    BtcLedgerMainnetHardwareImportScheme
  )

  const expectedResult: HardwareOperationError = unlockError
  expect(result).toEqual(expectedResult)
})
