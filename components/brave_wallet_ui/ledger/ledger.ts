/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { LedgerCommand, LEDGER_BRIDGE_URL } from '../common/hardware/ledgerjs/ledger-messages'
import { LedgerUntrustedMessagingTransport } from '../common/hardware/ledgerjs/ledger-untrusted-transport'

const setUpAuthorizeButtonListner = (targetUrl: string) => {
  const untrustedMessagingTransport = new LedgerUntrustedMessagingTransport(window.parent, targetUrl)
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

const targetUrl = new URLSearchParams(window.location.search).get('targetUrl')
if (targetUrl) {
  setUpAuthorizeButtonListner(targetUrl)
}
