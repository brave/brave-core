/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getMockChrome, getMockLoadTimeData } from './testData'

window.alert = jest.fn()

global.decodeURIComponent = () => 'test'

window.requestAnimationFrame = function (cb: FrameRequestCallback) {
  return window.setTimeout(cb, 0)
}

const windowAsAny = window as any

windowAsAny.chrome = getMockChrome()
windowAsAny.loadTimeData = getMockLoadTimeData()

windowAsAny.ResizeObserver = class ResizeObserverPolyfill {
  observe() {}

  unobserve() {}

  disconnect() {}
}

windowAsAny.IntersectionObserver = class IntersectionObserverPolyfill {
  observe() {}

  unobserve() {}

  disconnect() {}
}

if (typeof File.prototype.arrayBuffer !== 'function') {
  File.prototype.arrayBuffer = async function () {
    return new ArrayBuffer(0)
  }
}

// jsdom does not implement the Web Animations API. Svelte 5's transition system
// calls Element.animate() during mount/unmount and assigns onfinish to advance
// its state machine, so the stub fires onfinish on the next microtask.
Element.prototype.animate = function () {
  const animation = {
    cancel: () => {},
    finish: () => {},
    play: () => {},
    pause: () => {},
    reverse: () => {},
    onfinish: null as (() => void) | null,
    oncancel: null as (() => void) | null,
    finished: Promise.resolve(),
    addEventListener: () => {},
    removeEventListener: () => {}
  }
  queueMicrotask(() => animation.onfinish?.())
  return animation as unknown as Animation
}
