/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '$web-common/loadTimeData'
import * as TrezorMojom from 'gen/brave/components/brave_wallet/common/trezor_bridge.mojom.m.js' //

const kTrezorMojoFrameId = 'trezor-mojo-bridge-frame'

// Holds the single connection to the untrusted `chrome-untrusted://
// trezor-bridge` frame.
class TrezorBridgeRegistry {
  private bridge?: TrezorMojom.TrezorBridgeRemote

  getBridge = (): TrezorMojom.TrezorBridgeRemote => {
    if (!this.bridge) {
      this.bridge = this.connect()
    }
    return this.bridge
  }

  setBridgeForTesting = (remote: TrezorMojom.TrezorBridgeRemote) => {
    this.bridge = remote
  }

  resetForTesting = () => {
    this.bridge = undefined
    document.getElementById(kTrezorMojoFrameId)?.remove()
  }

  private connect = (): TrezorMojom.TrezorBridgeRemote => {
    // Create our end of the TrezorBridge pipe now and hand the receiver end
    // to the browser; it fuses it with the remote end the trezor subframe
    // hands up separately, so calls made on `remote` before fusing just queue
    // in the pipe rather than needing to wait for a round trip.
    const remote = new TrezorMojom.TrezorBridgeRemote()
    TrezorMojom.TrezorBridgeService.getRemote().bindTrezorBridge(
      remote.$.bindNewPipeAndPassReceiver(),
    )
    this.ensureFrame()
    return remote
  }

  private ensureFrame = () => {
    if (document.getElementById(kTrezorMojoFrameId)) {
      return
    }
    const url = loadTimeData.getString('braveWalletTrezorBridgeUrl')
    const element = document.createElement('iframe')
    element.id = kTrezorMojoFrameId
    element.src = url
    element.style.display = 'none'
    document.body.appendChild(element)
  }
}

// Shared singleton: all mojo trezor keyrings talk to the same bridge frame.
export const trezorBridgeRegistry = new TrezorBridgeRegistry()
