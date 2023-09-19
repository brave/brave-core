// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from './background/api/localeAPI'

const config = { attributes: true, childList: true, subtree: true }

const callback = () => {
  const buttonQueries = [
    'div.webstore-test-button-label', // https://chrome.google.com/webstore
    'button span[jsname]:not(:empty)' // https://chromewebstore.google.com/
  ]
  for (const button of document.querySelectorAll(buttonQueries.join(','))) {
    const text = button.textContent || ''
    if (text === getLocale('addToChrome')) {
      button.textContent =
        getLocale('addToBrave') || text.replace('Chrome', 'Brave')
    } else if (text === getLocale('removeFromChrome')) {
      button.textContent =
        getLocale('removeFromBrave') || text.replace('Chrome', 'Brave')
    }
  }
}

const observer: MutationObserver = new MutationObserver(callback)

// At https://chromewebstore.google.com, "*Chrome" buttons are displayed
// shortly after the first paint, and long before either `DOMContentLoaded` or
// `window.load` fire, so we must start watching for document mutations as soon
// as possible. The performance impact of observing mutations prior to `onload`
// is non-trivial but small (measured at 60ms on a development build).
observer.observe(document, config)
