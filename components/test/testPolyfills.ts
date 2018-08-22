/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getMockChrome } from './testData'

(global as any).window = {} as any

(window as any).localStorage = {
  getItem: jest.fn()
} as any

(window as any).location = {
  search: '?testTorrentId'
} as any

(window as any).decodeURIComponent = (any) => 'test'

// This mocks rAF to avoid React console.error
// while running Jest tests
(global as any).requestAnimationFrame = function (cb: () => void) {
  return setTimeout(cb, 0)
}

if ((global as any).chrome === undefined) {
  (global as any).chrome = getMockChrome()
}
