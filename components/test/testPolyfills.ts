/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

declare namespace NodeJS {
  interface Global {
    requestAnimationFrame: any
  }
}

(global as any).window = {} as any
(window as any).localStorage = {
  getItem: jest.fn()
}

// This mocks rAF to avoid React console.error
// while running Jest tests
global.requestAnimationFrame = function (cb: () => void) {
  return setTimeout(cb, 0)
}
