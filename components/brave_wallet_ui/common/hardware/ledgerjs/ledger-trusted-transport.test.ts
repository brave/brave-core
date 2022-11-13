/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LedgerTrustedMessagingTransport } from './ledger-trusted-transport'
import {
  LedgerCommand,
  AuthorizationSuccessCommand
} from './ledger-messages'

// We must read and write to protected class attributes in the tests.
// That yields a typescript error unless we use bracket notation, e.g. `transport['handlers']`
// instead of `transport.handlers`. As a result we silence the dot-notation
// tslint rule for the file.
//
/* eslint-disable @typescript-eslint/dot-notation */

const createWindow = (): Window => {
  let iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  if (!iframe.contentWindow) { fail('transport should be defined') }
  // Use Object.defineProperty in order to assign to
  // window.crypto because standard assignment results in
  // assignment error because window.origin is read-only
  Object.defineProperty(iframe.contentWindow, 'origin', {
    value: 'chrome-untrusted://ledger-bridge'
  })
  return iframe.contentWindow
}

test('handleAuthorizationSuccess calls onAuthorize callback', async () => {
  const targetWindow = createWindow()
  let callbackCalled = false
  const trustedTransport = new LedgerTrustedMessagingTransport(
    targetWindow,
    targetWindow.origin,
    () => { callbackCalled = true }
  )

  const command: AuthorizationSuccessCommand = {
    id: LedgerCommand.AuthorizationSuccess,
    origin: trustedTransport['targetWindow'].origin,
    command: LedgerCommand.AuthorizationSuccess
  }
  const event: MessageEvent = new MessageEvent('message', {
    data: command,
    origin: trustedTransport['targetWindow'].origin,
    source: trustedTransport['targetWindow']
  })
  window.dispatchEvent(event)
  expect(callbackCalled).toEqual(true)
})
