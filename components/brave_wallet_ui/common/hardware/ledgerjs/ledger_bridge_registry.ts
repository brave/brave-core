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
  private bridge?: Promise<LedgerMojom.LedgerBridgeRemote>

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
    document.getElementById(kLedgerMojoFrameId)?.remove()
  }

  private connect = async (): Promise<LedgerMojom.LedgerBridgeRemote> => {
    // Setup ledger subframe which will send LedgerBridge instance to browser.
    this.ensureFrame()
    // Wait for a pipe to ledger subframe.
    return (await LedgerMojom.LedgerBridgeService.getRemote().getLedgerBridge())
      .bridge
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
