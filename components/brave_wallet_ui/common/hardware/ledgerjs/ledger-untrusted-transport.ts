/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import { LedgerMessagingTransport } from './ledger-messaging-transport'
import { UnlockCommand, UnlockResponse } from './ledger-messages'

// LedgerUntrustedMessagingTransport is the messaging transport object
// for chrome-untrusted://ledger-bridge. It primarily handles postMessages
// coming from chrome://wallet or chrome://wallet-panel by making calls
// to Ledger libraries and replying with the results.
//
// We isolate the Ledger library from the wallet
// so that in the event it's compromised it will reduce the
// impact to the wallet.
export class LedgerUntrustedMessagingTransport //
  extends LedgerMessagingTransport
{
  constructor(targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
  }

  promptAuthorization = async () => {
    if (await this.authorizationNeeded()) {
      const transport = await TransportWebHID.create()
      await transport.close()
    }
  }

  protected handleUnlock = async (
    command: UnlockCommand
  ): Promise<UnlockResponse> => {
    const isAuthNeeded = await this.authorizationNeeded()
    if (isAuthNeeded) {
      return {
        ...command,
        payload: {
          success: false,
          error: 'unauthorized',
          code: 'unauthorized'
        }
      }
    }
    return {
      ...command,
      payload: {
        success: true
      }
    }
  }

  protected authorizationNeeded = async (): Promise<boolean> => {
    return (await TransportWebHID.list()).length === 0
  }
}
