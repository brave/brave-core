/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { assert } from 'chrome://resources/js/assert.m.js'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Eth from '@ledgerhq/hw-app-eth'

import { BraveWallet } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'
import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'
import {
  GetAccountsHardwareOperationResult,
  SignatureVRS,
  SignHardwareMessageOperationResult,
  SignHardwareTransactionOperationResult
, HardwareOperationResult, LedgerDerivationPaths
} from '../types'
import { LedgerEthereumKeyring } from '../interfaces'
import { HardwareVendor } from '../../api/hardware_keyrings'

export enum LedgerErrorsCodes {
  TransportLocked = 'TransportLocked'
}
export default class LedgerBridgeKeyring extends LedgerEthereumKeyring {
  constructor () {
    super()
  }

  private app?: Eth
  private deviceId: string

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.ETH
  }

  type = (): HardwareVendor => {
    return BraveWallet.LEDGER_HARDWARE_VENDOR
  }

  getAccounts = async (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult> => {
    const unlocked = await this.unlock()
    if (!unlocked.success || !this.app) {
      return unlocked
    }
    from = (from < 0) ? 0 : from
    const eth: Eth = this.app
    const accounts = []
    for (let i = from; i <= to; i++) {
      const path = this.getPathForIndex(i, scheme)
      const address = await eth.getAddress(path)
      accounts.push({
        address: address.address,
        derivationPath: path,
        name: this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId,
        coin: this.coin()
      })
    }
    return { success: true, payload: [...accounts] }
  }

  isUnlocked = (): boolean => {
    return this.app !== undefined
  }

  makeApp = async () => {
    this.app = new Eth(await TransportWebHID.create())
  }

  unlock = async (): Promise<HardwareOperationResult> => {
    if (this.app) {
      return { success: true }
    }

    await this.makeApp()
    if (!this.app) {
      return { success: false }
    }

    const eth: Eth = this.app
    eth.transport.on('disconnect', this.onDisconnected)
    const zeroPath = this.getPathForIndex(0, LedgerDerivationPaths.LedgerLive)
    const address = (await eth.getAddress(zeroPath)).address
    this.deviceId = await hardwareDeviceIdFromAddress(address)

    return { success: this.isUnlocked() }
  }

  signTransaction = async (path: string, rawTxHex: string): Promise<SignHardwareTransactionOperationResult> => {
    try {
      const unlocked = await this.unlock()
      if (!unlocked.success || !this.app) {
        return unlocked
      }
      const eth: Eth = this.app
      const signed = await eth.signTransaction(path, rawTxHex)
      return { success: true, payload: signed }
    } catch (e) {
      return { success: false, error: e.message, code: e.statusCode || e.id || e.name }
    }
  }

  signPersonalMessage = async (path: string, message: string): Promise<SignHardwareMessageOperationResult> => {
    try {
      const unlocked = await this.unlock()
      if (!unlocked.success || !this.app) {
        return unlocked
      }
      const eth: Eth = this.app
      const messageHex = Buffer.from(message).toString('hex')
      const data = await eth.signPersonalMessage(path, messageHex)
      const signature = this.createMessageSignature(data)
      if (!signature) {
        return { success: false, error: getLocale('braveWalletLedgerValidationError') }
      }
      return { success: true, payload: signature }
    } catch (e) {
      return { success: false, error: e.message, code: e.statusCode || e.id || e.name }
    }
  }

  cancelOperation = async () => {
    this.app?.transport.close()
  }

  private onDisconnected = (e: any) => {
    if (e.name !== 'DisconnectedDevice') {
      return
    }
    this.app = undefined
  }

  private readonly createMessageSignature = (result: SignatureVRS) => {
    let v = (result.v - 27).toString()
    if (v.length < 2) {
      v = `0${v}`
    }
    const signature = `0x${result.r}${result.s}${v}`
    return signature
  }

  private readonly getPathForIndex = (index: number, scheme: string): string => {
    if (scheme === LedgerDerivationPaths.LedgerLive) {
      return `m/44'/60'/${index}'/0/0`
    }
    assert(scheme === LedgerDerivationPaths.Legacy)
    return `m/44'/60'/${index}'/0`
  }
}
