/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

const createWindow = (): Window => {
  let iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  iframe.contentWindow.origin = 'chrome-untrusted://ledger-bridge'
  return iframe.contentWindow
}

test('constructor', async () => {
  const targetWindow = createWindow()
  const untrustedTransport = new LedgerUntrustedMessagingTransport(
    targetWindow,
    targetWindow.origin
  )
  expect(untrustedTransport.handlers.size).toEqual(3)
})
