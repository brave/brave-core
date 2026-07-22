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

// jsdom resolves computed styles by matching every stylesheet rule's selector
// with nwsapi, which cannot parse the `:has(input:where(...):checked)` selectors
// that Leo's Svelte components (e.g. leo-checkbox) register on the document.
// Such a rule makes nwsapi throw `... is not a valid selector` for *any*
// getComputedStyle call, aborting otherwise-unrelated work such as floating-ui
// measuring overflow ancestors. Fall back to an empty declaration so tests that
// render Leo components alongside these stylesheets can proceed.
const originalGetComputedStyle = window.getComputedStyle.bind(window)
window.getComputedStyle = function (...args) {
  try {
    return originalGetComputedStyle(...args)
  } catch (error) {
    if (
      error instanceof Error
      && error.message.includes('is not a valid selector')
    ) {
      return document.createElement('div').style
    }
    throw error
  }
} as typeof window.getComputedStyle

// jsdom does not implement the Constructable Stylesheets API used by
// components/common/scoped_css.ts to register component styles as a
// module-load side effect. Without this, importing any component whose
// sibling `.style.ts` file calls `scoped.css` crashes on
// `document.adoptedStyleSheets.push(...)` before a single test can run.
if (!('adoptedStyleSheets' in document)) {
  ;(document as any).adoptedStyleSheets = []
}
if (typeof (windowAsAny.CSSStyleSheet?.prototype as any)?.replace !== 'function') {
  windowAsAny.CSSStyleSheet = class CSSStyleSheetPolyfill {
    replace() {
      return Promise.resolve(this)
    }
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
