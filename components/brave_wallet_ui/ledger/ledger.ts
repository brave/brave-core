/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '../../common/loadTimeData'
import {
  BridgeType,
  BridgeTypes,
} from '../common/hardware/untrusted_shared_types'
import { LedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/ledger-untrusted-transport'
import { SolanaLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/sol-ledger-untrusted-transport'
import { EthereumLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/eth-ledger-untrusted-transport'
import { FilecoinLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/fil-ledger-untrusted-transport'
import { BitcoinLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/btc_ledger_untrusted_transport'
import { LedgerMojoUntrustedBridge } from './ledger_mojo_untrusted_bridge'
import * as LedgerMojom from 'gen/brave/components/brave_wallet/common/ledger_bridge.mojom.m.js' //

// Security: URL sanitization function to validate targetUrl
const checkWebuiScheme = (url: string): string | null => {
  const parsed = new URL(url)
  if (
    parsed.protocol === 'chrome-untrusted:'
    || parsed.protocol === `chrome:`
  ) {
    return url
  }
  return null
}

const setupUntrustedMessagingTransport = (
  bridgeType: string,
  targetUrl: string,
): LedgerUntrustedMessagingTransport => {
  switch (bridgeType as BridgeType) {
    case BridgeTypes.SolLedger:
      return new SolanaLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl,
      )
    case BridgeTypes.EthLedger:
      return new EthereumLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl,
      )
    case BridgeTypes.FilLedger:
      return new FilecoinLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl,
      )
    case BridgeTypes.BtcLedger:
      return new BitcoinLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl,
      )
    default:
      throw new Error(`Invalid bridgeType ${bridgeType}`)
  }
}

// Bootstraps the mojo transport: creates the device-backed `LedgerBridge`
// implementation, binds it to a mojo pipe, and hands the remote up to the
// browser, which routes it to the embedding wallet page/panel renderer.
const setupMojoBridge = () => {
  const bridge = new LedgerMojoUntrustedBridge()
  const receiver = new LedgerMojom.LedgerBridgeReceiver(bridge)
  const uiHandler = LedgerMojom.LedgerBridgeUIHandler.getRemote()
  uiHandler.bindLedgerBridge(receiver.$.bindNewPipeAndPassRemote())
}

if (loadTimeData.getBoolean('isLedgerMojoBridgeEnabled')) {
  setupMojoBridge()
} else {
  const params = new URLSearchParams(window.location.search)
  const targetUrl = params.get('targetUrl')
  const bridgeType = params.get('bridgeType')
  if (targetUrl && bridgeType) {
    const sanitizedUrl = checkWebuiScheme(targetUrl)
    if (sanitizedUrl) {
      setupUntrustedMessagingTransport(bridgeType, sanitizedUrl)
    }
  }
}
