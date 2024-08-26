/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '../../../../common/locale'
import LedgerBridgeKeyring from './ledger_bridge_keyring'
import {
  LedgerBridgeErrorCodes,
  LedgerFrameCommand,
  LedgerCommand,
  LedgerError,
  UnlockResponse,
  LedgerFrameResponse
} from './ledger-messages'
import { HardwareOperationResult } from '../types'
import { LedgerTrustedMessagingTransport } from './ledger-trusted-transport'

type MockResponseType = LedgerFrameResponse | LedgerBridgeErrorCodes

export class MockLedgerTransport extends LedgerTrustedMessagingTransport {
  sendCommandResponses: MockResponseType[] // queue

  constructor(
    targetWindow: Window,
    targetUrl: string,
    onAuthorized?: () => void
  ) {
    super(targetWindow, targetUrl)
    this.sendCommandResponses = []
  }

  addSendCommandResponse = (
    response: LedgerFrameResponse | LedgerBridgeErrorCodes
  ) => {
    // appends to the left of the list
    this.sendCommandResponses.unshift(response)
  }

  sendCommand = <T>(
    command: LedgerFrameCommand
  ): Promise<T | LedgerBridgeErrorCodes> => {
    const response = this.sendCommandResponses.pop()
    if (response === undefined) {
      throw new Error(
        'No mock ledger transport responses remaining.' +
          'More sendCommand calls were made than mocked responses added.'
      )
    }
    return new Promise((resolve) => resolve(response as T))
  }
}

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
      message: 'LedgerError',
      statusCode: 101
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
      message: 'LedgerError',
      statusCode: 101
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
    } as LedgerError
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
