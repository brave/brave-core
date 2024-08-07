/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getMockChrome, getMockLoadTimeData } from './testData'

window.alert = jest.fn()

window.location = {
  search: '?testTorrentId'
} as any

global.decodeURIComponent = () => 'test'

window.requestAnimationFrame = function (cb: FrameRequestCallback) {
  return window.setTimeout(cb, 0)
}

const windowAsAny = window as any

windowAsAny.chrome = getMockChrome()
windowAsAny.loadTimeData = getMockLoadTimeData()
