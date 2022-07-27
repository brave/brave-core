/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getLocale } from '../../../../common/locale'
import LedgerBridgeKeyring from './ledger_bridge_keyring'
import {
  LedgerBridgeErrorCodes,
  LedgerFrameCommand,
  LedgerFrameResponse
} from './ledger-messages'

let uuid = 0
window.crypto = {
  randomUUID () {
    return uuid++
  }
}

export class MockLedgerTransport {
  sendCommandResponses: LedgeFramerResponse[] // queue

  constructor () {
    this.sendCommandResponses = []
  }

  addSendCommandResponse = (response: LedgerFrameResponse) => {
    this.sendCommandResponses.unshift(response) // appends to the left of the list
  }

  sendCommand = async (command: LedgerFrameCommand): Promise<LedgerFrameResponse> => {
    if (this.sendCommandResponses.length === 0) {
      throw new Error('No mock ledger transport responses remaining. More sendCommand calls were made than mocked responses added.')
    }
    return this.sendCommandResponses.pop()
  }
}

const createKeyring = () => {
  const keyring = new LedgerBridgeKeyring()
  const transport = new MockLedgerTransport()
  keyring.transport = transport
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring.bridge = iframe

  return keyring
}

test('unlock successful', async () => {
  const keyring = createKeyring()
  const sendCommandResponse: UnlockResponsePayload = {
    payload: { success: true }
  }
  keyring.transport.addSendCommandResponse(sendCommandResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = sendCommandResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock ledger error', async () => {
  const keyring = createKeyring()
  const sendCommandResponse: UnlockResponsePayload = {
    payload: {
      message: 'LedgerError',
      statusCode: 101
    }
  }
  keyring.transport.addSendCommandResponse(sendCommandResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = sendCommandResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock unauthorized error', async () => {
  const keyring = createKeyring()
  const sendCommandResponse: UnlockResponsePayload = {
    payload: {
      success: false,
      error: 'unauthorized',
      code: undefined
    }
  }
  keyring.transport.addSendCommandResponse(sendCommandResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = sendCommandResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock bridge error', async () => {
  const keyring = createKeyring()
  keyring.transport.addSendCommandResponse(LedgerBridgeErrorCodes.BridgeNotReady)
  let result: HardwareOperationResult = await keyring.unlock()
  let expectedResult: HardwareOperationResult = {
    success: false,
    error: getLocale('braveWalletBridgeNotReady'),
    code: 0
  }
  expect(result).toEqual(expectedResult)

  keyring.transport.addSendCommandResponse(LedgerBridgeErrorCodes.CommandInProgress)
  result = await keyring.unlock()
  expectedResult = {
    success: false,
    error: getLocale('braveWalletBridgeCommandInProgress'),
    code: 1
  }
  expect(result).toEqual(expectedResult)
})
