// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Snap executor running inside chrome-untrusted://snap-executor/.
// Receives snap source from the parent wallet page via postMessage and
// evaluates it with new Function().
//
// All snaps share one realm and one origin (allow-same-origin sandbox) with
// no SES lockdown and no inter-snap isolation. Evaluated code has the frame's
// full globals (window.parent, postMessage, fetch, indexedDB, etc.).
// TODO(snaps): adopt MetaMask SES compartments per snaps_wiki.md.

import {
  ExecuteSnapPayload,
  isExecuteSnapCommand,
  SnapMessageType,
  WALLET_PAGE_ORIGIN,
} from '../common/snap/snap_messages'

function sendToParent(message: unknown) {
  window.parent.postMessage(message, WALLET_PAGE_ORIGIN)
}

function handleExecuteSnap(requestId: number, payload: ExecuteSnapPayload) {
  // Snap bundles are CommonJS, so provide `module`/`exports` and prefer an
  // explicit return value if the bundle produces one.
  const mod: { exports: unknown } = { exports: {} }
  try {
    const factory = new Function(
      'module',
      'exports',
      `"use strict";\n${payload.sourceCode}\n;return module.exports;`,
    )
    const exported = factory(mod, mod.exports)
    sendToParent({
      type: SnapMessageType.ExecuteSnapResult,
      requestId,
      success: true,
      error: null,
      result: typeof exported === 'string' ? exported : null,
    })
  } catch (err) {
    const error = err instanceof Error ? err.message : String(err)
    sendToParent({
      type: SnapMessageType.ExecuteSnapResult,
      requestId,
      success: false,
      error,
      result: null,
    })
  }
}

window.addEventListener('message', (event) => {
  if (event.origin !== WALLET_PAGE_ORIGIN || event.source !== window.parent) {
    return
  }
  if (!isExecuteSnapCommand(event.data)) {
    return
  }

  handleExecuteSnap(event.data.requestId, event.data.payload)
})

// Notify the parent that the executor is ready.
sendToParent({ type: SnapMessageType.ExecutorReady })
