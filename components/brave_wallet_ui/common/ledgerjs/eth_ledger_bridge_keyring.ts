/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

const { EventEmitter } = require('events')

import {
  HardwareWallet
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'

import Eth from '@ledgerhq/hw-app-eth'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'

export default class LedgerBridgeKeyring extends EventEmitter {
  constructor () {
    super()
  }

  type = () => {
    return HardwareWallet.Ledger
  }

  getAccounts = (from: number, to: number) => {
    return new Promise(async (resolve, reject) => {
      if (from < 0) {
        from = 0
      }
      if (!this.isUnlocked() && !(await this.unlock())) {
        return reject({ message: 'Unable to unlock device, try to reconnect' })
      }

      this._getAccountsBIP44(from, to).then(resolve).catch(reject)
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
    return this.app
  }

  /* PRIVATE METHODS */
  _getPathForIndex = (index: number) => {
    return `m/44'/60'/${index}'/0/0`
  }

  _getAddress = async (path: string) => {
    if (!this.isUnlocked()) {
      return null
    }
    return this.app.getAddress(path)
  }

  _getAccountsBIP44 = async (from: number, to: number) => {
    const accounts = []
    for (let i = from; i < to; i++) {
      const path = this._getPathForIndex(i)
      const address = await this._getAddress(path)
      accounts.push({
        address: address.address,
        derivationPath: path,
        balance: ''
      })
    }
    return accounts
  }
}
