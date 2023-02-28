// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from './background/api/localeAPI'

const config = { attributes: true, childList: true, subtree: true }

const callback = (mutationsList: MutationRecord[], observer: MutationObserver) => {
  const buttons = document.querySelectorAll('div.webstore-test-button-label')
  buttons.forEach((button: Element) => {
    const text = button.textContent || ''
    if (text === getLocale('addToChrome')) {
      button.textContent =
        getLocale('addToBrave') || text.replace('Chrome', 'Brave')
    } else if (text === getLocale('removeFromChrome')) {
      button.textContent =
        getLocale('removeFromBrave') || text.replace('Chrome', 'Brave')
    }
  })
}

const observer: MutationObserver = new MutationObserver(callback)

window.onload = () => {
  observer.observe(document, config)
}
