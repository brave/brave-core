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
  TrezorBridgeController
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
          return reject(new Error('Unable to unlock device, try to reconnect'))
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
    const result = (await this.bridge_.unlock())
    this.unlocked = result.success
    if (!result.success) {
      throw new Error(result.error);
    }
    return this.unlocked
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
      return `m/44'/60'/0'/${index}`
    } else {
      throw Error(`Unknown scheme: ${scheme}`)
    }
  }

  _getAccounts = async (from: number, to: number, scheme: string) => {
    const paths = []
    for (let i = from; i <= to; i++) {
      paths.push(this._getPathForIndex(i, scheme))
    }
    
    const accounts = await this.bridge_.getTrezorAccounts(paths)
    if (!accounts.success) {
      throw Error(accounts.error)
    }
    console.log(accounts.accounts)
    return accounts.accounts
  }
}
