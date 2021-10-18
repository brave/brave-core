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
  kLedgerHardwareVendor, SignatureVRS
} from '../../constants/types'

import Eth from '@ledgerhq/hw-app-eth'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import { getLocale } from '../../../common/locale'
import { recoverPersonalSignature } from 'eth-sig-util'
import { toChecksumAddress } from 'ethereumjs-util'

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
      this.deviceId_ = await this._getDeviceId(address)
    }
    return this.isUnlocked()
  }

  signTransaction = async (path: string, rawTxHex: string) => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return new Error(getLocale('braveWalletUnlockError'))
    }
    return this.app.signTransaction(path, rawTxHex)
  }

  signPersonalMessage = async (path: string, address: string, message: string) => {
    return new Promise(async (resolve, reject) => {
      try {
        if (!this.isUnlocked() && !(await this.unlock())) {
          return new Error(getLocale('braveWalletUnlockError'))
        }
        return this.app.signPersonalMessage(path,
          Buffer.from(message)).then((result: SignatureVRS) => {
            const signature = this._createMessageSignature(result, message, address)
            if (!signature) {
              return reject(new Error(getLocale('braveWalletLedgerValidationError')))
            }
            resolve(signature)
          }).catch(reject)
      } catch (e) {
        reject(e)
      }
    })
  }
  _recoverAddressFromSignature = (message: string, signature: string) => {
    return recoverPersonalSignature({ data: message, sig: signature })
  }
  _createMessageSignature = (result: SignatureVRS, message: string, address: string) => {
    let v = (result.v - 27).toString()
    if (v.length < 2) {
      v = `0${v}`
    }
    const signature = `0x${result.r}${result.s}${v}`
    const addressSignedWith = this._recoverAddressFromSignature(message, signature)
    if (toChecksumAddress(addressSignedWith) !== toChecksumAddress(address)) {
      return null
    }
    return signature
  }

  /* PRIVATE METHODS */
  _getDeviceId = async (address: string) => {
    const utf8 = new TextEncoder()
    const msgBuffer = utf8.encode(address)
    const hashBuffer = await Promise.resolve(crypto.subtle.digest('SHA-256', msgBuffer))
    const hashArray = Array.from(new Uint8Array(hashBuffer))
    const hashHex = hashArray.map(b => b.toString(16).padStart(2, '0')).join('')
    return hashHex
  }

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
