/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Transport from '@ledgerhq/hw-transport'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import { LedgerMessagingTransport } from './ledger-messaging-transport'
import {
  GetDeviceNameCommand,
  GetDeviceNameResponse,
  LedgerCommand,
  UnlockCommand,
  UnlockResponse,
} from './ledger-messages'

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
  private deviceName: string = ''

  constructor(targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)

    this.addCommandHandler<GetDeviceNameResponse>(
      LedgerCommand.GetDeviceName,
      this.handleGetDeviceName,
    )
  }

  private fillDeviceNameImpl = async (): Promise<void> => {
    try {
      const transport = await this.createTransport()
      const deviceName = transport.deviceModel?.productName ?? ''
      await transport.close()
      this.deviceName = deviceName
      console.log('this.deviceName', this.deviceName)
    } catch (error) {}
  }

  /** Verifies that a Ledger device is connected before creating a
   * transport for it. */
  protected createTransport = async (): Promise<Transport> => {
    const devices = await TransportWebHID.list()
    if (devices.length === 0) {
      throw new Error('No Ledger device found.')
    }
    return TransportWebHID.create()
  }

  private handleGetDeviceName = async (
    command: GetDeviceNameCommand,
  ): Promise<GetDeviceNameResponse> => {
    if (!this.deviceName) {
      await this.fillDeviceNameImpl()
    }
    return {
      ...command,
      payload: {
        success: true,
        deviceName: this.deviceName,
      },
    }
  }

  protected handleUnlock = async (
    command: UnlockCommand,
  ): Promise<UnlockResponse> => {
    await this.fillDeviceNameImpl()

    return {
      ...command,
      payload: {
        success: true,
      },
    }
  }
}
