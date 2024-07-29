/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Transport from '@ledgerhq/hw-transport'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Btc from '@ledgerhq/hw-app-btc'
import {
  BtcGetAccountCommand,
  BtcGetAccountResponse,
  BtcGetAccountResponsePayload,
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

/** makes calls to the Btc app on a Ledger device */
export class BitcoinLedgerUntrustedMessagingTransport //
  extends LedgerUntrustedMessagingTransport
{
  constructor(targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(
      LedgerCommand.Unlock,
      this.handleUnlock
    )
    this.addCommandHandler<BtcGetAccountResponse>(
      LedgerCommand.GetAccount,
      this.handleGetAccount
    )
  }

  private handleGetAccount = async (
    command: BtcGetAccountCommand
  ): Promise<BtcGetAccountResponse> => {
    let transport: Transport | undefined
    try {
      transport = await TransportWebHID.create()
      const app = new Btc({ transport })
      const result = await app.getWalletXpub({
        path: command.path,
        xpubVersion: command.xpubVersion
      })
      const getAccountResponsePayload: BtcGetAccountResponsePayload = {
        success: true,
        xpub: result
      }
      const response: BtcGetAccountResponse = {
        id: command.id,
        command: command.command,
        payload: getAccountResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: BtcGetAccountResponse = {
        id: command.id,
        command: command.command,
        payload: error,
        origin: command.origin
      }
      return response
    } finally {
      if (transport) {
        await transport.close()
      }
    }
  }
}
