/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import { LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import {
  LedgerProvider, TransportWrapper
} from '@glif/filecoin-wallet-provider'
import { CoinType } from '@glif/filecoin-address'
import { BraveWallet } from '../../../constants/types'
import { LedgerFilecoinKeyring } from '../interfaces'
import { HardwareVendor, getCoinName } from '../../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareMessageOperationResult,
  SignHardwareTransactionOperationResult
} from '../types'

export function encodeKeyToHex (key: string): string {
  return Buffer.from(key, 'base64').toString('hex')
}

export default class FilecoinLedgerKeyring implements LedgerFilecoinKeyring {
  private deviceId: string
  private provider?: LedgerProvider

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.FIL
  }

  type = (): HardwareVendor => {
    return LEDGER_HARDWARE_VENDOR
  }

  getAccounts = async (from: number, to: number, coinType?: CoinType): Promise<GetAccountsHardwareOperationResult> => {
    const unlocked = await this.unlock()
    if (!unlocked.success || !this.provider) {
      return unlocked
    }
    from = (from < 0) ? 0 : from
    const app: LedgerProvider = this.provider
    const accounts = await app.getAccounts(from, to, coinType)
    const result = []
    for (let i = 0; i < accounts.length; i++) {
      result.push({
        address: accounts[i],
        derivationPath: this.getPathForIndex(from + i),
        name: getCoinName(this.coin()) + ' ' + this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId,
        coin: this.coin()
      })
    }
    return { success: true, payload: [...result] }
  }

  isUnlocked = () => {
    return this.provider !== undefined
  }

  unlock = async (): Promise<HardwareOperationResult> => {
    if (this.provider) {
      return { success: true }
    }
    try {
      const transportWrapper = new TransportWrapper()
      await transportWrapper.connect()
      let provider = new LedgerProvider({
        transport: transportWrapper.transport,
        minLedgerVersion: {
          major: 0,
          minor: 0,
          patch: 1
        }
      })
      await provider.ready()

      const app: LedgerProvider = provider
      const address = await app.getAccounts(0, 1, CoinType.TEST)
      this.deviceId = address[0]
      transportWrapper.transport.on('disconnect', this.onDisconnected)
      this.provider = provider
      return { success: this.isUnlocked() }
    } catch (e) {
      return { success: false, error: e.message, code: e.statusCode || e.id || e.name }
    }
  }

  signPersonalMessage (path: string, address: string, message: string): Promise<SignHardwareMessageOperationResult> {
    throw new Error('Method not implemented.')
  }

  signTransaction (path: string, rawTxHex: string): Promise<SignHardwareTransactionOperationResult> {
    throw new Error('Method not implemented.')
  }

  private readonly getPathForIndex = (index: number): string => {
    return `m/44'/461'/0'/0/${index}`
  }

  private onDisconnected = (e: any) => {
    if (e.name !== 'DisconnectedDevice') {
      return
    }
    this.provider = undefined
  }
}
