/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  LedgerCommand,
  LEDGER_BRIDGE_URL
} from '../common/hardware/ledgerjs/ledger-messages'
import {
  BridgeType,
  BridgeTypes
} from '../common/hardware/untrusted_shared_types'
import { LedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/ledger-untrusted-transport'
import { SolanaLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/sol-ledger-untrusted-transport'
import { EthereumLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/eth-ledger-untrusted-transport'
import { FilecoinLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/fil-ledger-untrusted-transport'
import { BitcoinLedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/btc_ledger_untrusted_transport'

const setUpAuthorizeButtonListener = (
  targetUrl: string,
  bridgeType: string
) => {
  const untrustedMessagingTransport = getUntrustedMessagingTransport(
    bridgeType,
    targetUrl
  )
  window.addEventListener('DOMContentLoaded', (event) => {
    const authorizeBtn = document.getElementById('authorize')
    if (authorizeBtn) {
      authorizeBtn.addEventListener('click', () => {
        untrustedMessagingTransport.promptAuthorization().then((result) => {
          untrustedMessagingTransport.sendCommand({
            id: LedgerCommand.AuthorizationSuccess,
            origin: LEDGER_BRIDGE_URL,
            command: LedgerCommand.AuthorizationSuccess
          })
        })
      })
    }
  })
}

const getUntrustedMessagingTransport = (
  bridgeType: string,
  targetUrl: string
): LedgerUntrustedMessagingTransport => {
  switch (bridgeType as BridgeType) {
    case BridgeTypes.SolLedger:
      return new SolanaLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl
      )
    case BridgeTypes.EthLedger:
      return new EthereumLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl
      )
    case BridgeTypes.FilLedger:
      return new FilecoinLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl
      )
    case BridgeTypes.BtcLedger:
      return new BitcoinLedgerUntrustedMessagingTransport(
        window.parent,
        targetUrl
      )
    default:
      throw new Error(`Invalid bridgeType ${bridgeType}`)
  }
}

const params = new URLSearchParams(window.location.search)
const targetUrl = params.get('targetUrl')
const bridgeType = params.get('bridgeType')
if (targetUrl && bridgeType) {
  setUpAuthorizeButtonListener(targetUrl, bridgeType)
}
