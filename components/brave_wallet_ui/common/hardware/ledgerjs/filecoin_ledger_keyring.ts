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
  FilecoinNetwork,
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareOperationResult
} from '../types'
import { getLocale } from '../../../../common/locale'
import { LotusMessage, SignedLotusMessage } from '@glif/filecoin-message'

export default class FilecoinLedgerKeyring implements LedgerFilecoinKeyring {
  private deviceId: string
  private provider?: LedgerProvider
  private transportWrapper?: TransportWrapper

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.FIL
  }

  type = (): HardwareVendor => {
    return LEDGER_HARDWARE_VENDOR
  }

  getAccounts = async (from: number, to: number, network: FilecoinNetwork): Promise<GetAccountsHardwareOperationResult> => {
    const unlocked = await this.unlock()
    if (!unlocked.success || !this.provider) {
      return unlocked
    }
    from = (from < 0) ? 0 : from
    const app: LedgerProvider = this.provider
    const coinType = network === BraveWallet.FILECOIN_TESTNET ? CoinType.TEST : CoinType.MAIN
    const accounts = await app.getAccounts(from, to, coinType)
    const result = []
    for (let i = 0; i < accounts.length; i++) {
      result.push({
        address: accounts[i],
        derivationPath: this.getPathForIndex(from + i, coinType),
        name: getCoinName(this.coin()) + ' ' + this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId,
        coin: this.coin(),
        network: network
      })
    }
    return { success: true, payload: [...result] }
  }

  isUnlocked = () => {
    return this.provider !== undefined
  }

  makeApp = async (): Promise<Boolean> => {
    if (this.transportWrapper) {
      await this.transportWrapper.disconnect()
    }
    this.transportWrapper = new TransportWrapper()
    await this.transportWrapper.connect()
    let provider = new LedgerProvider({
      transport: this.transportWrapper.transport,
      minLedgerVersion: {
        major: 0,
        minor: 0,
        patch: 1
      }
    })
    if (!await provider.ready()) {
      return false
    }

    this.transportWrapper.transport.on('disconnect', this.onDisconnected)
    this.provider = provider
    return true
  }

  unlock = async (): Promise<HardwareOperationResult> => {
    if (this.provider) {
      return { success: true }
    }
    try {
      if (!await this.makeApp() || !this.provider) {
        return { success: false }
      }
      const app: LedgerProvider = this.provider
      const address = await app.getAccounts(0, 1, CoinType.TEST)
      this.deviceId = address[0]
      return { success: this.isUnlocked() }
    } catch (e) {
      return { success: false, error: getLocale('braveWalletLedgerFilecoinUnlockError') }
    }
  }

  signPersonalMessage (path: string, address: string, message: string): Promise<SignHardwareOperationResult> {
    throw new Error('Method not implemented.')
  }

  signTransaction = async (message: string): Promise<SignHardwareOperationResult> => {
    const unlocked = await this.unlock()
    if (!unlocked.success || !this.provider) {
      return { success: false, error: unlocked.error }
    }
    try {
      const parsed = JSON.parse(message)
      const lotusMessage: LotusMessage = {
        To: parsed.To,
        From: parsed.From,
        Nonce: parsed.Nonce,
        Value: parsed.Value,
        GasPremium: parsed.GasPremium,
        GasLimit: parsed.GasLimit,
        GasFeeCap: parsed.GasFeeCap,
        Method: parsed.Method,
        Params: parsed.Params
      }

      // Accounts must be warmed up before signing.
      await this.provider?.getAccounts()

      const signed: SignedLotusMessage = await this.provider?.sign(parsed.From, lotusMessage)
      return { success: true, payload: signed }
    } catch (e) {
      return { success: false, error: e.message, code: e.statusCode || e.id || e.name }
    }
  }

  private readonly getPathForIndex = (index: number, type: CoinType): string => {
    // According to SLIP-0044 For TEST networks coin type use 1 always.
    // https://github.com/satoshilabs/slips/blob/5f85bc4854adc84ca2dc5a3ab7f4b9e74cb9c8ab/slip-0044.md
    // https://github.com/glifio/modules/blob/primary/packages/filecoin-wallet-provider/src/utils/createPath/index.ts
    return type === CoinType.MAIN ? `m/44'/461'/0'/0/${index}` : `m/44'/1'/0'/0/${index}`
  }

  private onDisconnected = (e: any) => {
    if (e.name !== 'DisconnectedDevice') {
      return
    }
    this.provider = undefined
  }
}
