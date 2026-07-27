/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '../../../../common/loadTimeData'
import * as LedgerMojom //
  from 'gen/brave/components/brave_wallet/common/ledger_bridge.mojom.m.js'

const kLedgerMojoFrameId = 'ledger-mojo-bridge-frame'

// Holds the single connection to the untrusted `chrome-untrusted://
// ledger-bridge` frame (running in mojo mode). On first use it registers a
// `LedgerBridgeListener` with the browser (via `LedgerBridgeService`) and
// creates the hidden bridge iframe. When the child frame loads it hands its
// `LedgerBridge` remote up through the browser, which delivers it to
// `onLedgerBridgeConnected`, resolving `getBridge()`.
//
// The listener is registered before the iframe is created, and the browser-side
// broker buffers a pending bridge, so ordering is safe.
class LedgerBridgeRegistry {
  private bridge?: Promise<LedgerMojom.LedgerBridgeRemote>
  // Retained so the listener pipe is not garbage collected.
  private listenerReceiver?: LedgerMojom.LedgerBridgeListenerReceiver

  getBridge = (): Promise<LedgerMojom.LedgerBridgeRemote> => {
    if (!this.bridge) {
      this.bridge = this.connect()
    }
    return this.bridge
  }

  setBridgeForTesting = (remote: LedgerMojom.LedgerBridgeRemote) => {
    this.bridge = Promise.resolve(remote)
  }

  resetForTesting = () => {
    this.bridge = undefined
    this.listenerReceiver = undefined
    document.getElementById(kLedgerMojoFrameId)?.remove()
  }

  private connect = (): Promise<LedgerMojom.LedgerBridgeRemote> => {
    return new Promise((resolve) => {
      const listener: LedgerMojom.LedgerBridgeListenerInterface = {
        // A received pending_remote is delivered as an already-bound remote.
        onLedgerBridgeConnected: (bridge) => {
          resolve(bridge)
        },
      }
      this.listenerReceiver = new LedgerMojom.LedgerBridgeListenerReceiver(
        listener,
      )
      LedgerMojom.LedgerBridgeService.getRemote().registerLedgerBridgeListener(
        this.listenerReceiver.$.bindNewPipeAndPassRemote(),
      )
      this.ensureFrame()
    })
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
    element.setAttribute('sandbox', 'allow-scripts allow-same-origin')
    document.body.appendChild(element)
  }
}

// Shared singleton: all mojo ledger keyrings talk to the same bridge frame.
export const ledgerBridgeRegistry = new LedgerBridgeRegistry()
