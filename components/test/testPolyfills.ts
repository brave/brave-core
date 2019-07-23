/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { JSDOM } from 'jsdom'
import { getMockChrome } from './testData'

;(global as any).window = new JSDOM().window
;(global as any).document = window.document
;(global as any).navigator = window.navigator

window.location = {
  search: '?testTorrentId'
} as any

global.decodeURIComponent = () => 'test'

window.requestAnimationFrame = function (cb: FrameRequestCallback) {
  return window.setTimeout(cb, 0)
}

window.loadTimeData = {
  getString (key) {
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

console.timeStamp = function (key: string) { return }

if ((global as any).chrome === undefined) {
  (global as any).chrome = getMockChrome()
}
