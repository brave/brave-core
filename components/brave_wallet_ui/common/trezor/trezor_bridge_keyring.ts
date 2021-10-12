/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

const { EventEmitter } = require('events')
import {
  TrezorDerivationPaths, TrezorBridgeAccountsPayload
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'

import {
  kTrezorHardwareVendor
} from '../../constants/types'

import {
  kTrezorUnlockCommand,
  kTrezorGetAccountsCommand,
  kTrezorBridgeFrameId,
  kTrezorBridgeUrl
} from '../../common/trezor/constants'

import { getLocale } from '../../../common/locale'

const ethUtil = require('ethereumjs-util')

export default class TrezorBridgeKeyring extends EventEmitter {
  constructor () {
    super()
    this.unlocked_ = false
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
    return this.unlocked_
  }

  unlock = () => {
    return new Promise(async (resolve, reject) => {
      let bridge = document.getElementById(kTrezorBridgeFrameId) as HTMLIFrameElement
      if (!bridge) {
        bridge = await this._createBridge() as HTMLIFrameElement
      }
      const kUnlockEvent = { command: kTrezorUnlockCommand, id: new Date().getTime(), owner: window.origin }
      if (!bridge.contentWindow) {
        throw Error(getLocale('braveWalletCreateBridgeError'))
      }
      this.addEventListener(kUnlockEvent.id, (data: any) => {
        this.unlocked_ = data.result
        if (data.result) {
          resolve(true)
        } else {
          reject(false)
        }
      })
      bridge.contentWindow.postMessage(kUnlockEvent, kTrezorBridgeUrl)
    })
  }

  addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived.bind(this))
  }

  removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived.bind(this))
  }

  addEventListener = (id: number, listener: Function) => {
    if (!this.pending_requests_) {
      this.addWindowMessageListener()
      this.pending_requests_ = {}
    }
    if (this.pending_requests_.hasOwnProperty(id)) {
      return false
    }
    this.pending_requests_[id] = listener
    return true
  }

  removeEventListener = (id: number) => {
    if (!this.pending_requests_.hasOwnProperty(id)) {
      return false
    }
    delete this.pending_requests_[id]
    if (!Object.keys(this.pending_requests_).length) {
      this.pending_requests_ = null
      this.removeWindowMessageListener()
    }
    return true
  }

  onMessageReceived = (event: any) => {
    if (!event.origin.startsWith(kTrezorBridgeUrl) || event.type !== 'message') {
      return
    }
    if (!event.data || !this.pending_requests_ ||
        !this.pending_requests_.hasOwnProperty(event.data.id)) {
      return
    }
    const callback = this.pending_requests_[event.data.id] as Function
    callback.call(this, event.data)
    this.removeEventListener(event.data.id)
  }

  _getBridge = () => {
    return document.getElementById(kTrezorBridgeFrameId)
  }

  _getTrezorAccounts = async (paths: string[]): Promise<TrezorBridgeAccountsPayload> => {
    return new Promise(async (resolve, reject) => {
      const bridge = this._getBridge() as HTMLIFrameElement
      if (!bridge) {
        reject(Error(getLocale('braveWalletCreateBridgeError')))
        return
      }
      const requestedPaths = []
      for (const path of paths) {
        requestedPaths.push({ path: path })
      }
      const kGetAccountEvent = { command: kTrezorGetAccountsCommand, id: new Date().getTime(), paths: requestedPaths, owner: window.origin }
      if (!bridge.contentWindow) {
        throw Error(getLocale('braveWalletCreateBridgeError'))
      }
      this.addEventListener(kGetAccountEvent.id, (data: any) => {
        if (data.payload.success) {
          let accounts = []
          for (const value of data.payload.payload) {
            const buffer = Buffer.from(value.publicKey, 'hex')
            const address = ethUtil.publicToAddress(buffer, true).toString('hex')
            accounts.push({
              address: ethUtil.toChecksumAddress(`0x${address}`),
              derivationPath: value.serializedPath,
              name: this.type(),
              hardwareVendor: this.type(),
              deviceId: String(value.fingerprint)
            })
          }
          resolve({ success: true, accounts: [...accounts] })
        } else {
          reject({ success: false, error: getLocale('braveWalletImportingAccountsError') })
        }
      })
      bridge.contentWindow.postMessage(kGetAccountEvent, kTrezorBridgeUrl)
    })
  }

  _createBridge = async () => {
    return new Promise(async (resolve, reject) => {
      let element = document.createElement('iframe')
      element.id = kTrezorBridgeFrameId
      element.src = kTrezorBridgeUrl
      element.style.display = 'none'
      element.onload = () => {
        resolve(element)
      }
      document.body.appendChild(element)
    })
  }

  _getPathForIndex = (index: number, scheme: string) => {
    if (scheme === TrezorDerivationPaths.Default) {
      return `m/44'/60'/0'/${index}`
    } else {
      throw Error(getLocale('braveWalletDeviceUnknownScheme'))
    }
  }

  _getAccounts = async (from: number, to: number, scheme: string) => {
    const paths = []
    for (let i = from; i <= to; i++) {
      paths.push(this._getPathForIndex(i, scheme))
    }

    const accounts = await this._getTrezorAccounts(paths)
    if (!accounts.success) {
      throw Error(accounts.error)
    }
    return accounts.accounts
  }
}
