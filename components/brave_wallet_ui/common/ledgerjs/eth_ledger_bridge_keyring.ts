/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

const { EventEmitter } = require('events')

import {
  LedgerDerivationPaths
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'

import {
  kLedgerHardwareVendor, SignatureVRS,
  SignHardwareMessageOperationResult
} from '../../constants/types'

import Eth from '@ledgerhq/hw-app-eth'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import { getLocale } from '../../../common/locale'
import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'

export default class LedgerBridgeKeyring extends EventEmitter {
  constructor () {
    super()
  }

  type = () => {
    return kLedgerHardwareVendor
  }

  getAccounts = (from: number, to: number, scheme: string) => {
    return new Promise(async (resolve, reject) => {
      if (from < 0) {
        from = 0
      }
      try {
        if (!this.isUnlocked() && !(await this.unlock())) {
          return reject(new Error(getLocale('braveWalletUnlockError')))
        }
      } catch (e) {
        reject(e)
        return
      }
      this._getAccounts(from, to, scheme).then(resolve).catch(reject)
    })
  }

  isUnlocked = () => {
    return this.app !== undefined
  }

  unlock = async () => {
    if (this.app) {
      return this.app
    }
    this.app = new Eth(await TransportWebHID.create())
    if (this.app) {
      const zeroPath = this._getPathForIndex(0, LedgerDerivationPaths.LedgerLive)
      const address = await this._getAddress(zeroPath)
      this.deviceId_ = await hardwareDeviceIdFromAddress(address)
    }
    return this.isUnlocked()
  }

  signTransaction = async (path: string, rawTxHex: string) => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return new Error(getLocale('braveWalletUnlockError'))
    }
    return this.app.signTransaction(path, rawTxHex)
  }

  signPersonalMessage = async (path: string, address: string, message: string): Promise<SignHardwareMessageOperationResult> => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return { success: false, error: getLocale('braveWalletUnlockError') }
    }
    try {
      const data = await this.app.signPersonalMessage(path,
        Buffer.from(message))
      const signature = this._createMessageSignature(data, message, address)
      if (!signature) {
        return { success: false, error: getLocale('braveWalletLedgerValidationError') }
      }
      return { success: true, payload: signature }
    } catch (e) {
      console.log(e)
      return { success: false, error: e.message }
    }
  }

  _createMessageSignature = (result: SignatureVRS, message: string, address: string) => {
    let v = (result.v - 27).toString()
    if (v.length < 2) {
      v = `0${v}`
    }
    const signature = `0x${result.r}${result.s}${v}`
    return signature
  }

  /* PRIVATE METHODS */
  _getPathForIndex = (index: number, scheme: string) => {
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

  _getAccounts = async (from: number, to: number, scheme: string) => {
    const accounts = []
    for (let i = from; i <= to; i++) {
      const path = this._getPathForIndex(i, scheme)
      const address = await this._getAddress(path)
      accounts.push({
        address: address.address,
        derivationPath: path,
        name: this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId_
      })
    }
    return accounts
  }
}
