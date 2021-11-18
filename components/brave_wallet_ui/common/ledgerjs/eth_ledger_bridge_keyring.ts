/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const { EventEmitter } = require('events')
import Eth from '@ledgerhq/hw-app-eth'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import { LEDGER_HARDWARE_VENDOR } from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import {
  LedgerDerivationPaths
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { getLocale } from '../../../common/locale'
import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'
import { GetAccountsHardwareOperationResult, SignatureVRS, SignHardwareMessageOperationResult, SignHardwareTransactionOperationResult } from '../../common/hardware_operations'

export enum LedgerErrorsCodes {
  TransportLocked = 'TransportLocked'
}

export default class LedgerBridgeKeyring extends EventEmitter {
  constructor () {
    super()
  }

  type = () => {
    return LEDGER_HARDWARE_VENDOR
  }

  getAccounts = async (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult> => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return { success: false, error: getLocale('braveWalletUnlockError') }
    }
    if (from < 0) {
      from = 0
    }
    const accounts = []
    for (let i = from; i <= to; i++) {
      const path = this.getPathForIndex(i, scheme)
      const address = await this._getAddress(path)
      accounts.push({
        address: address.address,
        derivationPath: path,
        name: this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId_
      })
    }
    return { success: true, payload: [...accounts] }
  }

  isUnlocked = () => {
    return this.app !== undefined
  }

  makeApp = async () => {
    this.app = new Eth(await TransportWebHID.create())
  }

  unlock = async () => {
    if (this.app) {
      return this.app
    }

    await this.makeApp()

    if (this.app) {
      const zeroPath = this.getPathForIndex(0, LedgerDerivationPaths.LedgerLive)
      const address = await this._getAddress(zeroPath)
      this.deviceId_ = await hardwareDeviceIdFromAddress(address)
    }
    return this.isUnlocked()
  }

  signTransaction = async (path: string, rawTxHex: string): Promise<SignHardwareTransactionOperationResult> => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return { success: false, error: getLocale('braveWalletUnlockError') }
    }
    const signed = await this.app.signTransaction(path, rawTxHex)
    return { success: true, payload: signed }
  }

  signPersonalMessage = async (path: string, message: string): Promise<SignHardwareMessageOperationResult> => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return { success: false, error: getLocale('braveWalletUnlockError') }
    }
    try {
      const data = await this.app.signPersonalMessage(path,
        Buffer.from(message))
      const signature = this.createMessageSignature(data)
      if (!signature) {
        return { success: false, error: getLocale('braveWalletLedgerValidationError') }
      }
      return { success: true, payload: signature }
    } catch (e) {
      return { success: false, error: e.message, code: e.id }
    }
  }

  private readonly createMessageSignature = (result: SignatureVRS) => {
    let v = (result.v - 27).toString()
    if (v.length < 2) {
      v = `0${v}`
    }
    const signature = `0x${result.r}${result.s}${v}`
    return signature
  }

  /* PRIVATE METHODS */
  private readonly getPathForIndex = (index: number, scheme: string) => {
    if (scheme === LedgerDerivationPaths.LedgerLive) {
      return `m/44'/60'/${index}'/0/0`
    } else if (scheme === LedgerDerivationPaths.Legacy) {
      return `m/44'/60'/${index}'/0`
    } else {
      throw Error(getLocale('braveWalletDeviceUnknownScheme'))
    }
  }

  _getAddress = async (path: string) => {
    if (!this.isUnlocked()) {
      return null
    }
    return this.app.getAddress(path)
  }
}
