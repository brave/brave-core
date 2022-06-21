/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { LedgerTrustedMessagingTransport } from './ledger-trusted-transport'
import {
  LedgerCommand,
  AuthorizationSuccessCommand
} from './ledger-messages'

const createWindow = (): Window => {
  let iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  iframe.contentWindow.origin = 'chrome-untrusted://ledger-bridge'
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
    origin: trustedTransport.targetWindow.origin,
    command: LedgerCommand.AuthorizationSuccess
  }
  const event: MessageEvent = new MessageEvent('message', {
    data: command,
    origin: trustedTransport.targetWindow.origin,
    source: trustedTransport.targetWindow
  })
  window.dispatchEvent(event)
  expect(callbackCalled).toEqual(true)
})
