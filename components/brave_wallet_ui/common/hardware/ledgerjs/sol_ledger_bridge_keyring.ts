/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { BraveWallet } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'
import { LedgerSolanaKeyring } from '../interfaces'
import { HardwareVendor } from '../../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareOperationResult
} from '../types'
import {
  LEDGER_BRIDGE_URL,
  LedgerCommand,
  UnlockResponse,
  LedgerFrameCommand,
  GetAccountResponse,
  GetAccountResponsePayload,
  SignTransactionResponse,
  SignTransactionResponsePayload,
  LedgerErrorsCodes,
  LedgerError
} from './ledger-messages'
import { LedgerTrustedMessagingTransport } from './ledger-trusted-transport'
import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'

// storybook compiler thinks `randomUUID` doesn't exist
const randomUUID = () => (window.crypto as Crypto & { randomUUID: () => string }).randomUUID()
export default class SolanaLedgerBridgeKeyring implements LedgerSolanaKeyring {
  private deviceId: string
  private onAuthorized?: () => void
  private transport?: LedgerTrustedMessagingTransport
  private bridge?: HTMLIFrameElement
  private readonly frameId: string

  constructor (onAuthorized?: () => void) {
    this.onAuthorized = onAuthorized
    this.frameId = randomUUID()
  }

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.SOL
  }

  type = (): HardwareVendor => {
    return LEDGER_HARDWARE_VENDOR
  }

  unlock = async (): Promise<HardwareOperationResult> => {
    const data = await this.sendCommand<UnlockResponse>({
      id: LedgerCommand.Unlock,
      origin: window.origin,
      command: LedgerCommand.Unlock
    })

    if (data === LedgerErrorsCodes.BridgeNotReady ||
        data === LedgerErrorsCodes.CommandInProgress) {
      return this.createErrorFromCode(data)
    }

    return data.payload
  }

  getAccounts = async (from: number, to: number): Promise<GetAccountsHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    from = (from >= 0) ? from : 0
    const paths = []
    const addZeroPath = (from > 0 || to < 0)
    if (addZeroPath) {
      // Add zero address to calculate device id.
      paths.push(this.getPathForIndex(0))
    }
    for (let i = from; i <= to; i++) {
      paths.push(this.getPathForIndex(i))
    }
    return this.getAccountsFromDevice(paths, addZeroPath)
  }

  signTransaction = async (path: string, rawTxBytes: Buffer): Promise<SignHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }

    const data = await this.sendCommand<SignTransactionResponse>({
      command: LedgerCommand.SignTransaction,
      id: LedgerCommand.SignTransaction,
      path: path,
      rawTxBytes: rawTxBytes,
      origin: window.origin
    })
    if (data === LedgerErrorsCodes.BridgeNotReady ||
        data === LedgerErrorsCodes.CommandInProgress) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      // TODO Either pass data.payload (LedgerError) or data.payload.message (LedgerError.message)
      // consistently here and in getAccountsFromDevice.  Currently we pass the entire LedgerError up
      // to UI only for getAccounts to make statusCode available, but don't do the same here
      // for signTransaction.
      const ledgerError = data.payload as LedgerError
      return { success: false, error: ledgerError.message, code: ledgerError.statusCode }
    }
    const responsePayload = data.payload as SignTransactionResponsePayload
    return { success: true, payload: responsePayload.signature }
  }

  private readonly createBridge = (targetUrl: string) => {
    return new Promise<HTMLIFrameElement>((resolve) => {
      const element = document.createElement('iframe')
      element.id = this.frameId
      element.src = (new URL(targetUrl)).origin + `?targetUrl=${encodeURIComponent(window.origin)}`
      element.style.display = 'none'
      element.allow = 'hid'
      element.onload = () => {
        this.bridge = element
        resolve(element)
      }
      document.body.appendChild(element)
    })
  }

  private readonly getAccountsFromDevice = async (paths: string[], skipZeroPath: boolean): Promise<GetAccountsHardwareOperationResult> => {
    let accounts = []
    const zeroPath = this.getPathForIndex(0)
    for (const path of paths) {
      const data = await this.sendCommand<GetAccountResponse>({
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        path: path,
        origin: window.origin
      })
      if (data === LedgerErrorsCodes.BridgeNotReady ||
          data === LedgerErrorsCodes.CommandInProgress) {
        return this.createErrorFromCode(data)
      }

      if (!data.payload.success) {
        const ledgerError = data.payload as LedgerError
        return { success: false, error: ledgerError, code: ledgerError.statusCode }
      }
      const responsePayload = data.payload as GetAccountResponsePayload

      if (path === zeroPath) {
        this.deviceId = await hardwareDeviceIdFromAddress(responsePayload.address)
        if (skipZeroPath) {
          // If requested addresses do not have zero indexed adress we add it
          // intentionally to calculate device id and should not add it to
          // returned accounts
          continue
        }
      }

      accounts.push({
        address: '',
        addressBytes: responsePayload.address,
        derivationPath: path,
        name: this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId,
        coin: this.coin(),
        network: undefined
      })
    }
    return { success: true, payload: accounts }
  }

  sendCommand = async <T> (command: LedgerFrameCommand): Promise<T | LedgerErrorsCodes > => {
    if (!this.bridge && !this.hasBridgeCreated()) {
      this.bridge = await this.createBridge(LEDGER_BRIDGE_URL)
    }
    if (!this.bridge || !this.bridge.contentWindow) {
      return LedgerErrorsCodes.BridgeNotReady
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

  cancelOperation = async () => { }

  private readonly createErrorFromCode = (code: LedgerErrorsCodes): HardwareOperationResult => {
    switch (code) {
      case LedgerErrorsCodes.BridgeNotReady:
        return { success: false, error: getLocale('braveWalletBridgeNotReady'), code: code }
      case LedgerErrorsCodes.CommandInProgress:
        return { success: false, error: getLocale('braveWalletBridgeCommandInProgress'), code: code }
    }
  }

  private readonly getPathForIndex = (index: number): string => {
    return `44'/501'/${index}'/0'`
  }

  private readonly hasBridgeCreated = (): boolean => {
    return document.getElementById(this.frameId) !== null
  }
}
