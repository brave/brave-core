/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import tabActions from '../actions/tabActions'

chrome.webNavigation.onCommitted.addListener(
  (details: chrome.webNavigation.WebNavigationFramedCallbackDetails) => {
    tabActions.tabUpdated(details.tabId, details)
  },
  { url: [
    { pathSuffix: '.torrent' },
    // have a magnet scheme
    { schemes: ['magnet'] }]
  }
 )

chrome.webNavigation.onCommitted.addListener(
  (details: chrome.webNavigation.WebNavigationFramedCallbackDetails) => {
    // navigation events come in with the webtorrent extension as
    // the schema/host, the actual webtorrent URL appended to it
    // after the chrome-extenion://extension-id/view.html?
    // delete everything up to the first question mark
    let original = details.url.replace(new RegExp(/^[^\?]*\?/), '')
    let decoded = decodeURIComponent(original)
    details.url = decoded
    tabActions.tabUpdated(details.tabId, details)
  },
  { url: [
    // when handling URLs as chrome-extension://...
    // the actual torrent URL is within query parameters
    { schemes: ['chrome-extension'], queryContains: '.torrent' },
    { schemes: ['chrome-extension'], queryContains: 'magnet%3A' }]
  }
)

chrome.tabs.onRemoved.addListener(
  (tabId: number, removeInfo: chrome.tabs.TabRemoveInfo) => {
    tabActions.tabRemoved(tabId)
  }
)
