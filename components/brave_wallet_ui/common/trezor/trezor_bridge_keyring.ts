/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

const { EventEmitter } = require('events')

import {
  TrezorDerivationPaths
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'

import {
  kTrezorHardwareVendor,
  TrezorBridgeController,
  TrezorBridgeGetAddressReturnInfo
} from '../../constants/types'

export default class TrezorBridgeKeyring extends EventEmitter {
  constructor (bridge: TrezorBridgeController) {
    super()
    this.bridge_ = bridge
  }

  type = () => {
    return kTrezorHardwareVendor
  }

  getAccounts = (from: number, to: number, scheme: string) => {
    return new Promise(async (resolve, reject) => {
      if (from < 0) {
        from = 0
      }
      try {
        if (!this.isUnlocked() && !(await this.unlock())) {
          return reject(new Error())
        }
      } catch (e) {
        reject(e)
        return
      }
      this._getAccounts(from, to, scheme).then(resolve).catch(reject)
    })
  }

  isUnlocked = () => {
    return this.unlocked
  }

  unlock = async () => {
    this.unlocked = (await this.bridge_.unlock()).success
    return this.isUnlocked()
  }

  signTransaction = async (path: string, rawTxHex: string) => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return new Error('Unable to unlock device, try to reconnect')
    }
    return this.bridge_.signTransaction(path, rawTxHex)
  }

  /* PRIVATE METHODS */
  _getPathForIndex = (index: number, scheme: string) => {
    if (scheme === TrezorDerivationPaths.Default) {
      return `m/49'/0'/${index}'`
    } else {
      throw Error(`Unknown scheme: ${scheme}`)
    }
  }

  _getAddress = async (path: string):Promise<TrezorBridgeGetAddressReturnInfo> => {
    if (!this.isUnlocked()) {
      return Promise.reject({succes: false, address: ''})
    }
    return this.bridge_.getAddress(path)
  }

  _getAccounts = async (from: number, to: number, scheme: string) => {
    const accounts = []
    for (let i = from; i <= to; i++) {
      const path = this._getPathForIndex(i, scheme)
      const address = await this._getAddress(path)
      if (!address.success) {
        continue
      }
      accounts.push({
        address: address.address,
        derivationPath: path,
        name: this.type() + ' ' + i,
        hardwareVendor: this.type()
      })
    }
    return accounts
  }
}
