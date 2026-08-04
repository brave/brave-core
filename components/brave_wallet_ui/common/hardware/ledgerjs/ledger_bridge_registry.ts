/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '../../../../common/loadTimeData'
import * as LedgerMojom from 'gen/brave/components/brave_wallet/common/ledger_bridge.mojom.m.js' //

const kLedgerMojoFrameId = 'ledger-mojo-bridge-frame'

// Holds the single connection to the untrusted `chrome-untrusted://
// ledger-bridge` frame.
class LedgerBridgeRegistry {
  private bridge?: LedgerMojom.LedgerBridgeRemote

  getBridge = (): LedgerMojom.LedgerBridgeRemote => {
    if (!this.bridge) {
      this.bridge = this.connect()
    }
    return this.bridge
  }

  setBridgeForTesting = (remote: LedgerMojom.LedgerBridgeRemote) => {
    this.bridge = remote
  }

  resetForTesting = () => {
    this.bridge = undefined
    document.getElementById(kLedgerMojoFrameId)?.remove()
  }

  private connect = (): LedgerMojom.LedgerBridgeRemote => {
    // Create our end of the LedgerBridge pipe now and hand the receiver end
    // to the browser; it fuses it with the remote end the ledger subframe
    // hands up separately, so calls made on `remote` before fusing just queue
    // in the pipe rather than needing to wait for a round trip.
    const remote = new LedgerMojom.LedgerBridgeRemote()
    LedgerMojom.LedgerBridgeService.getRemote().bindLedgerBridge(
      remote.$.bindNewPipeAndPassReceiver(),
    )
    this.ensureFrame()
    return remote
  }

  private ensureFrame = () => {
    if (document.getElementById(kLedgerMojoFrameId)) {
      return
    }
    const url = loadTimeData.getString('braveWalletLedgerBridgeUrl')
    const element = document.createElement('iframe')
    element.id = kLedgerMojoFrameId
    element.src = url
    element.style.display = 'none'
    element.allow = 'hid'
    // @ledgerhq/* needs allow-scripts+allow-same-origin; cross-scheme iframe
    // from chrome:// to chrome-untrusted:// keeps isolation but prevents
    // opaque origin issues.
    element.setAttribute('sandbox', 'allow-scripts allow-same-origin')
    document.body.appendChild(element)
  }
}

// Shared singleton: all mojo ledger keyrings talk to the same bridge frame.
export const ledgerBridgeRegistry = new LedgerBridgeRegistry()
