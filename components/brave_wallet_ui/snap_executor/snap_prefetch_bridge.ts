// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// snap_prefetch_bridge.ts — tiny shim that runs inside the snap executor iframe
// BEFORE MetaMask's IFrameSnapExecutor bundle.
//
// The wallet page (chrome://wallet) cannot fetch from chrome-untrusted:// or
// use chrome:// scheme with fetch(). This shim allows the wallet page to ask
// the iframe to fetch a snap bundle from its own origin (which has connect-src *)
// and return the source code via postMessage.
//
// Protocol:
//   Parent → iframe:  { type: 'brave_prefetch_bundle', url: string, requestId: string }
//   iframe → Parent:  { type: 'brave_prefetch_bundle_response', requestId: string, source?: string, error?: string }
//
// MetaMask's WindowPostMessageStream only processes messages with
// { target: 'child', data: ... } format, so these messages don't interfere.

// Capture globals immediately, before MetaMask's bundle.js runs.
// LavaMoat scuttles ALL globalThis properties (except a small allow-list) after
// bundle.js loads, making fetch, parent, postMessage, etc. inaccessible.
const snapParentWindow = window.parent
const snapFetch = window.fetch.bind(window)
const snapConsole = window.console
const snapPostMessage = snapParentWindow.postMessage.bind(snapParentWindow)
snapConsole.error('XXXZZZ snap_prefetch_bridge: loaded, parent=', !!snapParentWindow)

window.addEventListener('message', async (event: MessageEvent) => {
  // Note: event.source is nullified by executeLockdownEvents() inside bundle.js,
  // so we cannot check it here. The message type + requestId uniquely identify
  // our messages; other message types (MetaMask streams) have different shapes.
  const data = event.data as Record<string, unknown>
  if (!data || data.type !== 'brave_prefetch_bundle') {
    return
  }
  const requestId = data.requestId as string
  const url = data.url as string
  snapConsole.error('XXXZZZ snap_prefetch_bridge: fetching', url)
  try {
    const resp = await snapFetch(url)
    if (!resp.ok) {
      throw new Error(`HTTP ${resp.status} ${resp.statusText}`)
    }
    const source = await resp.text()
    snapConsole.error('XXXZZZ snap_prefetch_bridge: fetched', url, 'length=', source.length)
    snapPostMessage(
      { type: 'brave_prefetch_bundle_response', requestId, source },
      '*',
    )
  } catch (err) {
    const msg = err instanceof Error ? err.message : String(err)
    snapPostMessage(
      { type: 'brave_prefetch_bundle_response', requestId, error: msg },
      '*',
    )
  }
})
