// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'
import { SnapHostBridge } from './snap_host_bridge'

class SnapHostBridgeRegistry {
  private bridge?: SnapHostBridge

  ensureBridge = (): SnapHostBridge => {
    if (!this.bridge) {
      const bridge = new SnapHostBridge()
      this.bridge = bridge
      this.connect(bridge)
    }
    return this.bridge
  }

  private connect = (bridge: SnapHostBridge) => {
    try {
      BraveWallet.SnapService.getRemote().bindSnapHostBridge(bridge.bind())
    } catch (err) {
      // Snaps stay unavailable for this page; there is nothing to retry
      // against until it reloads.
      console.error('Failed to bind SnapHostBridge', err)
      if (this.bridge === bridge) {
        this.bridge = undefined
      }
    }
  }

  setBridgeForTesting = (bridge: SnapHostBridge) => {
    this.bridge = bridge
  }

  resetForTesting = () => {
    this.bridge?.close()
    this.bridge = undefined
  }
}

export const snapHostBridgeRegistry = new SnapHostBridgeRegistry()
