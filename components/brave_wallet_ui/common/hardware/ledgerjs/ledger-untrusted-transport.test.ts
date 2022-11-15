/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

// To use the LedgerUntrustedMessagingTransport, we must read
// the protected `handlers` attribute, which yields a typescript
// error unless we use bracket notation. As a result we must
// silence the dot-notation tslint rule for the file.
//
/* eslint-disable @typescript-eslint/dot-notation */

const createWindow = (): Window => {
  let iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  Object.defineProperty(iframe.contentWindow, 'origin', {
    value: 'chrome-untrusted://ledger-bridge'
  })
  if (!iframe.contentWindow) { fail('transport should be defined') }
  return iframe.contentWindow
}

test('constructor', async () => {
  const targetWindow = createWindow()
  const untrustedTransport = new LedgerUntrustedMessagingTransport(
    targetWindow,
    targetWindow.origin
  )
  expect(untrustedTransport['handlers'].size).toEqual(0)
})
