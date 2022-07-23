/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Transport from '@ledgerhq/hw-transport'
import Sol from '@ledgerhq/hw-app-solana'
import { BraveWallet } from '../../../constants/types'
import { LedgerSolanaKeyring } from '../interfaces'
import { HardwareVendor } from '../../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareOperationResult
} from '../types'

import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'

export default class SolanaLedgerBridgeKeyring implements LedgerSolanaKeyring {
  private app?: Sol
  private transport?: Transport
  private deviceId: string

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.SOL
  }

  type = (): HardwareVendor => {
    return LEDGER_HARDWARE_VENDOR
  }

  getAccounts = async (from: number, to: number): Promise<GetAccountsHardwareOperationResult> => {
    const unlocked = await this.unlock()
    if (!unlocked.success || !this.app) {
      return unlocked
    }
    from = (from < 0) ? 0 : from
    const sol: Sol = this.app
    const accounts = []
    for (let i = from; i <= to; i++) {
      const path = this.getPathForIndex(i)
      const address = await sol.getAddress(path)
      accounts.push({
        address: '',
        addressBytes: address.address,
        derivationPath: path,
        name: this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId,
        coin: this.coin(),
        network: undefined
      })
    }
    return { success: true, payload: [...accounts] }
  }

  isUnlocked = (): boolean => {
    return this.app !== undefined && this.deviceId !== undefined
  }

  makeApp = async () => {
    this.transport = await TransportWebHID.create()
    this.app = new Sol(this.transport)
  }

  unlock = async (): Promise<HardwareOperationResult> => {
    if (this.isUnlocked()) {
      return { success: true }
    }

    if (!this.app) {
      await this.makeApp()
    }

    if (this.app && !this.deviceId) {
      const sol: Sol = this.app
      this.transport?.on('disconnect', this.onDisconnected)
      const zeroPath = this.getPathForIndex(0)
      const address = (await sol.getAddress(zeroPath)).address
      this.deviceId = await hardwareDeviceIdFromAddress(address)
    }
    return { success: this.isUnlocked() }
  }

  signTransaction = async (path: string, rawTxBytes: Buffer): Promise<SignHardwareOperationResult> => {
    try {
      const unlocked = await this.unlock()
      if (!unlocked.success || !this.app) {
        return unlocked
      }
      const sol: Sol = this.app
      const signed = await sol.signTransaction(path, rawTxBytes)
      return { success: true, payload: signed.signature }
    } catch (e) {
      return { success: false, error: e.message, code: e.statusCode || e.id || e.name }
    }
  }

  cancelOperation = async () => {
    this.transport?.close()
  }

  private onDisconnected = (e: any) => {
    if (e.name !== 'DisconnectedDevice') {
      return
    }
    this.app = undefined
    this.transport = undefined
  }

  private readonly getPathForIndex = (index: number): string => {
    return `44'/501'/${index}'/0'`
  }
}
