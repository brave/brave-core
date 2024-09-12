/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '../../../../common/locale'
import LedgerBridgeKeyring from './ledger_bridge_keyring'
import {
  LedgerBridgeErrorCodes,
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import { HardwareOperationResult } from '../types'
import { MockLedgerTransport } from './mock_ledger_transport'

const createKeyring = () => {
  const keyring = new LedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  keyring.setTransportForTesting(transport)
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring.setBridgeForTesting(iframe)

  return { keyring, transport }
}

test('unlock successful', async () => {
  const { keyring, transport } = createKeyring()

  const unlockResponse: UnlockResponse = {
    id: LedgerCommand.Unlock,
    origin: window.origin,
    command: LedgerCommand.Unlock,
    payload: {
      success: false,
      error: 'LedgerError',
      code: 101
    }
  }
  transport.addSendCommandResponse(unlockResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = unlockResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock ledger error', async () => {
  const { keyring, transport } = createKeyring()

  const unlockResponse: UnlockResponse = {
    id: LedgerCommand.Unlock,
    origin: window.origin,
    command: LedgerCommand.Unlock,
    payload: {
      success: false,
      error: 'LedgerError',
      code: 101
    }
  }
  transport.addSendCommandResponse(unlockResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = unlockResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock unauthorized error', async () => {
  const { keyring, transport } = createKeyring()

  const sendCommandResponse: UnlockResponse = {
    id: LedgerCommand.Unlock,
    origin: window.origin,
    command: LedgerCommand.Unlock,
    payload: {
      success: false,
      error: 'unauthorized',
      code: undefined
    }
  }
  transport.addSendCommandResponse(sendCommandResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = sendCommandResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock bridge error123', async () => {
  const { keyring, transport } = createKeyring()

  transport.addSendCommandResponse(LedgerBridgeErrorCodes.BridgeNotReady)
  let result: HardwareOperationResult = await keyring.unlock()
  let expectedResult: HardwareOperationResult = {
    success: false,
    error: getLocale('braveWalletBridgeNotReady'),
    code: 0
  }
  expect(result).toEqual(expectedResult)

  transport.addSendCommandResponse(LedgerBridgeErrorCodes.CommandInProgress)
  result = await keyring.unlock()
  expectedResult = {
    success: false,
    error: getLocale('braveWalletBridgeCommandInProgress'),
    code: 1
  }
  expect(result).toEqual(expectedResult)
})
