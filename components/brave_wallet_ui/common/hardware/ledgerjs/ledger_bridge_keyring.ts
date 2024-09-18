/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BridgeType } from '../untrusted_shared_types'
import { getLocale } from '../../../../common/locale'
import { HardwareOperationError, HardwareOperationResult } from '../types'
import {
  LEDGER_BRIDGE_URL,
  LedgerCommand,
  UnlockResponse,
  LedgerFrameCommand,
  LedgerBridgeErrorCodes
} from './ledger-messages'
import { LedgerTrustedMessagingTransport } from './ledger-trusted-transport'

// storybook compiler thinks `randomUUID` doesn't exist
const randomUUID = () =>
  (window.crypto as Crypto & { randomUUID: () => string }).randomUUID()

// LedgerBridgeKeyring is the parent class for the various BridgeKeyrings, e.g.
// SolanaLedgerBridgeKeyring
export default class LedgerBridgeKeyring {
  protected onAuthorized?: () => void
  protected transport?: LedgerTrustedMessagingTransport
  protected bridge?: HTMLIFrameElement
  protected readonly frameId: string

  constructor(onAuthorized?: () => void) {
    this.onAuthorized = onAuthorized
    this.frameId = randomUUID()
  }

  setTransportForTesting = (transport: LedgerTrustedMessagingTransport) => {
    this.transport = transport
  }

  setBridgeForTesting = (bridge: HTMLIFrameElement) => {
    this.bridge = bridge
  }

  bridgeType = (): BridgeType => {
    throw new Error('Unimplemented.')
  }

  unlock = async (): Promise<HardwareOperationResult> => {
    const data = await this.sendCommand<UnlockResponse>({
      id: LedgerCommand.Unlock,
      origin: window.origin,
      command: LedgerCommand.Unlock
    })

    if (
      data === LedgerBridgeErrorCodes.BridgeNotReady ||
      data === LedgerBridgeErrorCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      return { ...data.payload }
    }

    return { success: true }
  }

  sendCommand = async <T>(
    command: LedgerFrameCommand
  ): Promise<T | LedgerBridgeErrorCodes> => {
    if (!this.bridge && !this.hasBridgeCreated()) {
      this.bridge = await this.createBridge(LEDGER_BRIDGE_URL)
    }
    if (!this.bridge || !this.bridge.contentWindow) {
      return LedgerBridgeErrorCodes.BridgeNotReady
    }
    if (!this.transport) {
      this.transport = new LedgerTrustedMessagingTransport(
        this.bridge.contentWindow,
        LEDGER_BRIDGE_URL,
        this.onAuthorized
      )
    }
    return this.transport.sendCommand(command)
  }

  cancelOperation = async () => {}

  protected readonly createBridge = (targetUrl: string) => {
    return new Promise<HTMLIFrameElement>((resolve) => {
      const element = document.createElement('iframe')
      element.id = this.frameId
      element.src =
        new URL(targetUrl).origin +
        `?targetUrl=${encodeURIComponent(window.origin)}` +
        `&bridgeType=${this.bridgeType()}`
      element.style.display = 'none'
      element.allow = 'hid'
      element.setAttribute('sandbox', 'allow-scripts allow-same-origin')
      element.onload = () => {
        this.bridge = element
        resolve(element)
      }
      document.body.appendChild(element)
    })
  }

  protected readonly createErrorFromCode = (
    code: LedgerBridgeErrorCodes
  ): HardwareOperationError => {
    const deviceName = getLocale('braveWalletConnectHardwareLedger')

    switch (code) {
      case LedgerBridgeErrorCodes.BridgeNotReady:
        return {
          success: false,
          error: getLocale('braveWalletBridgeNotReady').replace(
            '$1',
            deviceName
          ),
          code: code
        }
      case LedgerBridgeErrorCodes.CommandInProgress:
        return {
          success: false,
          error: getLocale('braveWalletBridgeCommandInProgress').replace(
            '$1',
            deviceName
          ),
          code: code
        }
    }
  }

  protected readonly hasBridgeCreated = (): boolean => {
    return document.getElementById(this.frameId) !== null
  }
}
