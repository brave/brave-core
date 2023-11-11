/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { randomFillSync, randomUUID } from 'crypto'
import { JSDOM } from 'jsdom'
import { getMockChrome } from './testData'

;(global as any).window = new JSDOM().window
;(global as any).document = window.document
;(global as any).navigator = window.navigator

window.location = {
  search: '?testTorrentId'
} as any

window.requestAnimationFrame = function (cb: FrameRequestCallback) {
  return window.setTimeout(cb, 0)
}

window.loadTimeData = {
  getString (key) {
    if (key === 'braveWalletLedgerBridgeUrl') {
      return 'chrome-untrusted://ledger-bridge'
    }
    return key
  },
  getBoolean () {
    return true
  },
  getInteger () {
    return 2
  },
  getStringF (key) {
    return key
  }
}

Object.defineProperty(window, 'crypto', {
  value: {
    getRandomValues: (buffer: any) => randomFillSync(buffer),
    randomUUID
  }
})

console.timeStamp = function (key: string) { }

if ((global as any).chrome === undefined) {
  (global as any).chrome = getMockChrome()
}
