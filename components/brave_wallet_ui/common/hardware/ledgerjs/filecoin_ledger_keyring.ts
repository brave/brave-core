/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import * as bls from '@noble/bls12-381'
import { LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import {
  LedgerProvider, TransportWrapper
} from '@glif/filecoin-wallet-provider'
import { CoinType } from '@glif/filecoin-address'
import { FilecoinKeyring } from '../hardwareKeyring'
import { HardwareVendor, HardwareCoins, getCoinName } from '../../api/hardware_keyrings'
import {
  GetAccountsHardwareOperationResult,
  HardwareOperationResult,
  SignHardwareMessageOperationResult,
  SignHardwareTransactionOperationResult
} from '../../hardware_operations'

function switchEndianness (hexString: string): string | false {
  const regex = hexString.match(/.{2}/g)
  if (!regex) {
    return false
  }
  return regex.reverse().join('')
}

export function extractPublicKeyForBLS (privateKey: string): string {
  // https://github.com/brave/brave-browser/issues/20024
  const reversedKey = switchEndianness(privateKey)
  if (!reversedKey) {
    return ''
  }
  return Buffer.from(bls.getPublicKey(reversedKey)).toString('hex')
}

export function encodeKeyToHex (key: string): string {
  return Buffer.from(key, 'base64').toString('hex')
}

export default class FilecoinLedgerKeyring extends FilecoinKeyring {
  constructor () {
    super()
  }

  private deviceId: string
  private provider?: LedgerProvider

  coin = (): BraveWallet.BraveCoins => {
    return BraveWallet.BraveCoins.FILECOIN
  }

  type = (): HardwareVendor => {
    return BraveWallet.LEDGER_HARDWARE_VENDOR
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
        derivationPath: (from + i).toString(),
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
      this.provider = new LedgerProvider({
        transport: transportWrapper.transport,
        minLedgerVersion: {
          major: 0,
          minor: 0,
          patch: 1
        }
      })
      await this.provider.ready()
      if (!this.provider) {
        return { success: false }
      }

      const app: LedgerProvider = this.provider
      const address = await app.getAccounts(0, 1, CoinType.TEST)
      this.deviceId = address[0]
      transportWrapper.transport.on('disconnect', this.onDisconnected)
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

  private onDisconnected = (e: any) => {
    if (e.name !== 'DisconnectedDevice') {
      return
    }
    this.provider = undefined
  }
}
