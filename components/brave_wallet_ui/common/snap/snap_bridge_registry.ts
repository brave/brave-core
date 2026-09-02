// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'
import { SnapBridge } from './snap_bridge'

class SnapBridgeRegistry {
  private bridge?: SnapBridge

  ensureBridge = (): SnapBridge => {
    if (!this.bridge) {
      const service = BraveWallet.SnapsService.getRemote()
      const bridge = new SnapBridge()
      bridge.setSnapsService(service)
      service.setSnapBridge(bridge.bindNewPipeAndPassRemote())
      const handler = new BraveWallet.SnapRequestHandlerRemote()
      service.bindSnapRequestHandler(handler.$.bindNewPipeAndPassReceiver())
      bridge.setSnapRequestHandler(handler)
      this.bridge = bridge
    }
    return this.bridge
  }

  setBridgeForTesting = (bridge: SnapBridge) => {
    this.bridge = bridge
  }

  resetForTesting = () => {
    this.bridge = undefined
  }
}

export const snapBridgeRegistry = new SnapBridgeRegistry()
