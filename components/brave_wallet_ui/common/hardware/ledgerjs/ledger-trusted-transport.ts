/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LedgerMessagingTransport } from './ledger-messaging-transport'
import {
  LedgerCommand,
  AuthorizationSuccessCommand,
  AuthorizationSuccessResponsePayload
} from './ledger-messages'

// LedgerTrustedMessagingTransport is the messaging transport object
// for chrome://wallet and chrome://wallet-panel. It makes calls to the Ledger
// libraries isolated in chrome-untrusted://ledger-bridge via postMessage,
// and defines a handler to be run when the untrusted instance is granted access
// to the user's the Ledger device
export class LedgerTrustedMessagingTransport extends LedgerMessagingTransport {
  private onAuthorized?: () => void

  constructor(
    targetWindow: Window,
    targetUrl: string,
    onAuthorized?: () => void
  ) {
    super(targetWindow, targetUrl)
    this.onAuthorized = onAuthorized
    this.addCommandHandler(
      LedgerCommand.AuthorizationSuccess,
      this.handleAuthorizationSuccess
    )
  }

  private handleAuthorizationSuccess = async (
    command: AuthorizationSuccessCommand
  ): Promise<AuthorizationSuccessResponsePayload> => {
    if (this.onAuthorized) {
      this.onAuthorized()
    }

    return {
      ...command,
      payload: {
        success: true
      }
    }
  }
}
