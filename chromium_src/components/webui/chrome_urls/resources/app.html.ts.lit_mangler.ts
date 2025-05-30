/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { mangle } from 'lit_mangler'

// Two sets of templates that we need to mangle are indistinguishable from each
// other on the HTML level, so we distinguish them via their mangler-assigned
// unique IDs
let seenAnchorElementId: number | undefined
let seenListItemId: number | undefined

function performRewriteForAnchorElement(element) {
  const anchor = element.querySelector('a')
  if (!anchor) {
    throw new Error('[chrome_urls override] Missing anchor element')
  }
  if (anchor.textContent !== '\${info.url.url}') {
    throw new Error('[chrome_urls override] Unexpected anchor textContent')
  }
  anchor.textContent = '\${info.url.url.replace(/chrome:/, "brave:")}'
}

function performRewriteForListItem(element) {
  const listItem = element.querySelector('li')
  if (!listItem) {
    throw new Error('[chrome_urls override] Missing list item element')
  }
  if (listItem.textContent !== '\${info.url.url}') {
    throw new Error('[chrome_urls override] Unexpected list item textContent')
  }
  listItem.textContent = '\${info.url.url.replace(/chrome:/, "brave:")}'
}

// Rewrite H2 text content
mangle((element) => {
  const h2 = element.querySelector('h2')
  if (!h2) {
    throw new Error('[chrome_urls override] Missing H2 element')
  }
  h2.textContent = 'List of Brave URLs'
})

// Rewrite chrome://chrome-urls -> brave://chrome-urls
mangle((element) => {
  const anchor = element.querySelector('a')
  if (!anchor) {
    throw new Error('[chrome_urls override] Missing anchor element')
  }
  if (anchor.textContent !== 'chrome://chrome-urls') {
    throw new Error('[chrome_urls override] Unexpected anchor textContent')
  }
  anchor.textContent = 'brave://chrome-urls'
}, x => x.text.includes('href="#"'))

// Rewrite standard chrome URLs to use brave: scheme (these appear under the
// "List of Brave URLs" header)
mangle((element) => {
  performRewriteForAnchorElement(element)
}, x => {
  if (x.text.includes('href="${info.url.url}"')) {
    seenAnchorElementId = x.id
    return true
  }
  return false
})

// Rewrite internal debugging page URLs to use brave: scheme (these appear under
// the "Internal Debugging Page URLs" header when the debugging pages are
// enabled)
mangle((element) => {
  performRewriteForAnchorElement(element)
}, x => x.text.includes('href="${info.url.url}"') &&
      seenAnchorElementId !== x.id)

// Rewrite inactive chrome URLs to use brave: scheme (these also appear under
// the "List of Brave URLs" header)
mangle((element) => {
  performRewriteForListItem(element)
}, x => {
  if (x.text.includes('<li>${info.url.url}</li>')) {
    seenListItemId = x.id
    return true
  }
  return false
})

// Rewrite internal debugging page URLs to use brave: scheme (these appear under
// the "Internal Debugging Page URLs" header when the debugging pages are
// disabled)
mangle((element) => {
  performRewriteForListItem(element)
}, x => x.text.includes('<li>${info.url.url}</li>') &&
      (seenListItemId !== x.id))

// Rewrite command URLs to use brave: scheme (these appear under the
// "Command URLs for Debug" header)
mangle((element) => {
  const listItem = element.querySelector('li')
  if (!listItem) {
    throw new Error('[chrome_urls override] Missing list item element')
  }
  if (listItem.textContent !== '\${url.url}') {
    throw new Error('[chrome_urls override] Unexpected list item textContent')
  }
  listItem.textContent = '\${url.url.replace(/chrome:/, "brave:")}'
}, x => x.text.includes('<li>${url.url}</li>'))
