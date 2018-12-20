/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getMockChrome } from './testData'

(global as any).window = {} as any

(window as any).localStorage = {
  getItem: jest.fn(),
  setItem: jest.fn()
} as any

window.location = {
  search: '?testTorrentId'
} as any

global.decodeURIComponent = () => 'test'

window.requestAnimationFrame = function (cb: FrameRequestCallback) {
  return window.setTimeout(cb, 0)
}

if ((global as any).chrome === undefined) {
  (global as any).chrome = getMockChrome()
}
