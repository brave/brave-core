/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '../../../../common/locale'
import LedgerBridgeKeyring from './ledger_bridge_keyring'
import {
  LedgerBridgeErrorCodes,
  LedgerFrameCommand,
  LedgerFrameResponse,
  LedgerCommand,
  LedgerError,
  UnlockResponse
} from './ledger-messages'
import { HardwareOperationResult } from '../types'
import { LedgerTrustedMessagingTransport } from './ledger-trusted-transport'

export class MockLedgerTransport extends LedgerTrustedMessagingTransport {
  sendCommandResponses: any [] // queue

  constructor (targetWindow: Window, targetUrl: string, onAuthorized?: () => void) {
    super(targetWindow, targetUrl)
    this.sendCommandResponses = []
  }

 addSendCommandResponse = (response: LedgerFrameResponse) => {
    this.sendCommandResponses.unshift(response) // appends to the left of the list
  }

  sendCommand = <T> (command: LedgerFrameCommand): Promise<T | LedgerBridgeErrorCodes> => {
    if (this.sendCommandResponses.length === 0) {
      throw new Error('No mock ledger transport responses remaining. More sendCommand calls were made than mocked responses added.')
    }
    const response = this.sendCommandResponses.pop()
    return response
  }
}

// To use the MockLedgerTransport, we must overwrite
// the protected `transport` attribute, which yields a typescript
// error unless we use bracket notation, i.e. keyring['transport']
// instead of keyring.transport. As a result we silence the dot-notation
// tslint rule for the file.
//
/* eslint-disable @typescript-eslint/dot-notation */

const createKeyring = () => {
  const keyring = new LedgerBridgeKeyring()
  const transport = new MockLedgerTransport(window, window.origin)
  keyring['transport'] = transport
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  keyring['bridge'] = iframe

  return keyring
}

test('unlock successful', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
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
  keyring['transport']['addSendCommandResponse'](unlockResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = unlockResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock ledger error', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
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
  keyring['transport']['addSendCommandResponse'](unlockResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = unlockResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock unauthorized error', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
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
  keyring['transport']['addSendCommandResponse'](sendCommandResponse)
  const result: HardwareOperationResult = await keyring.unlock()
  const expectedResult: HardwareOperationResult = sendCommandResponse.payload
  expect(result).toEqual(expectedResult)
})

test('unlock bridge error', async () => {
  const keyring = createKeyring()
  if (!keyring['transport']) { fail('transport should be defined') }
  keyring['transport']['addSendCommandResponse'](LedgerBridgeErrorCodes.BridgeNotReady)
  let result: HardwareOperationResult = await keyring.unlock()
  let expectedResult: HardwareOperationResult = {
    success: false,
    error: getLocale('braveWalletBridgeNotReady'),
    code: 0
  }
  expect(result).toEqual(expectedResult)

  keyring['transport']['addSendCommandResponse'](LedgerBridgeErrorCodes.CommandInProgress)
  result = await keyring.unlock()
  expectedResult = {
    success: false,
    error: getLocale('braveWalletBridgeCommandInProgress'),
    code: 1
  }
  expect(result).toEqual(expectedResult)
})
