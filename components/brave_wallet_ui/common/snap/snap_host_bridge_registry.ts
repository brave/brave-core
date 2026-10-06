// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'
import { SnapHostBridge } from './snap_host_bridge'

// The browser drops its remote whenever it tears the bridge down — most often
// on wallet lock. Re-binding keeps snaps working after unlock without a page
// reload. Bounded so a pipe that fails immediately on every attempt (e.g. the
// service is gone) can't spin.
const MAX_REBIND_ATTEMPTS = 5

class SnapHostBridgeRegistry {
  private bridge?: SnapHostBridge
  private rebindAttempts = 0

  ensureBridge = (): SnapHostBridge => {
    if (this.bridge) {
      return this.bridge
    }
    // connect() can clear this.bridge synchronously, so return the local ref.
    const bridge = new SnapHostBridge()
    this.bridge = bridge
    this.connect(bridge)
    return bridge
  }

  private connect = (bridge: SnapHostBridge) => {
    try {
      BraveWallet.SnapService.getRemote().bindSnapHostBridge(
        bridge.bind(() => this.onDisconnect(bridge)),
      )
    } catch (err) {
      // Snaps stay unavailable for this page; there is nothing to retry
      // against until it reloads.
      console.error('Failed to bind SnapHostBridge', err)
      if (this.bridge === bridge) {
        this.bridge = undefined
      }
    }
  }

  // The browser closed its end. Drop the stale snap iframes and re-bind a
  // fresh bridge so the next LoadSnap has somewhere to land.
  private onDisconnect = (bridge: SnapHostBridge) => {
    if (this.bridge !== bridge) {
      return
    }
    // A bridge that served at least one request was a healthy one, so this
    // disconnect is a fresh teardown rather than a repeated bind failure.
    if (bridge.didServeRequest) {
      this.rebindAttempts = 0
    }
    bridge.close()
    this.bridge = undefined

    if (this.rebindAttempts >= MAX_REBIND_ATTEMPTS) {
      console.error('SnapHostBridge disconnected too often; giving up')
      return
    }
    this.rebindAttempts++
    this.ensureBridge()
  }

  setBridgeForTesting = (bridge: SnapHostBridge) => {
    this.bridge = bridge
  }

  resetForTesting = () => {
    this.bridge?.close()
    this.bridge = undefined
    this.rebindAttempts = 0
  }
}

export const snapHostBridgeRegistry = new SnapHostBridgeRegistry()
