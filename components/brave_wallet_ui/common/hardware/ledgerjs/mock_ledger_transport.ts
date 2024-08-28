/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  LedgerBridgeErrorCodes,
  LedgerFrameCommand,
  LedgerFrameResponse
} from './ledger-messages'
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
