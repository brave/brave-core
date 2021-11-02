/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

const { EventEmitter } = require('events')
import { publicToAddress, toChecksumAddress } from 'ethereumjs-util'
import {
  TrezorDerivationPaths, TrezorBridgeAccountsPayload
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import {
  kTrezorHardwareVendor
} from '../../constants/types'
import {
  TrezorCommand,
  TrezorFrameCommand,
  kTrezorBridgeFrameId,
  kTrezorBridgeUrl,
  UnlockCommand,
  GetAccountsCommand,
  UnlockResponse,
  GetAccountsResponsePayload,
  TrezorAccount,
  postToTrezorFrame,
  TrezorError
} from '../../common/trezor/trezor-messages'
import { getLocale } from '../../../common/locale'

export default class TrezorBridgeKeyring extends EventEmitter {
  constructor () {
    super()
    this.unlocked_ = false
  }

  type = () => {
    return kTrezorHardwareVendor
  }

  getAccounts = async (from: number, to: number, scheme: string) => {
    if (from < 0) {
      from = 0
    }
    if (!this.isUnlocked() && !(await this.unlock())) {
      return new Error(getLocale('braveWalletUnlockError'))
    }
    return this._getAccounts(from, to, scheme)
  }

  isUnlocked = () => {
    return this.unlocked_
  }

  unlock = () => {
    return new Promise(async (resolve, reject) => {
      // @ts-ignore
      const messageId: string = crypto.randomUUID()
      this.addEventListener(messageId, (data: UnlockResponse) => {
        this.unlocked_ = data.result
        if (data.result) {
          resolve(true)
        } else {
          reject(false)
        }
      })
      const message: UnlockCommand = { id: messageId, origin: window.origin,
        command: TrezorCommand.Unlock }
      await this.postMessage(message)
    })
  }

  private postMessage = async (command: TrezorFrameCommand) => {
    let bridge = this.getBridge() as HTMLIFrameElement
    if (!bridge) {
      bridge = await this._createBridge() as HTMLIFrameElement
    }
    if (!bridge.contentWindow) {
      throw Error(getLocale('braveWalletCreateBridgeError'))
    }
    postToTrezorFrame(bridge.contentWindow, command)
  }

  private addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived.bind(this))
  }

  private removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived.bind(this))
  }
  private getTrezorBridgeOrigin = () => {
    return (new URL(kTrezorBridgeUrl)).origin
  }
  private addEventListener = (id: string, listener: Function) => {
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

  private removeEventListener = (id: number) => {
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

  private onMessageReceived = (event: any /* MessageEvent<TrezorFrameResponse> */) => {
    if (event.origin !== this.getTrezorBridgeOrigin() || event.type !== 'message') {
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

  private getBridge = () => {
    return document.getElementById(kTrezorBridgeFrameId)
  }

  private getTrezorAccounts = async (paths: string[]): Promise<TrezorBridgeAccountsPayload> => {
    return new Promise(async (resolve, reject) => {
      const requestedPaths = []
      for (const path of paths) {
        requestedPaths.push({ path: path })
      }
      // @ts-ignore
      const messageId: string = crypto.randomUUID()
      this.addEventListener(messageId, (data: GetAccountsResponsePayload) => {
        if (data.payload.success) {
          let accounts = []
          for (const value of data.payload.payload as TrezorAccount[]) {
            const buffer = Buffer.from(value.publicKey, 'hex')
            const address = publicToAddress(buffer, true).toString('hex')
            accounts.push({
              address: toChecksumAddress(`0x${address}`),
              derivationPath: value.serializedPath,
              name: this.type(),
              hardwareVendor: this.type(),
              deviceId: String(value.fingerprint)
            })
          }
          resolve({ success: true, accounts: [...accounts] })
        } else {
          const error = data.payload.payload as TrezorError
          reject({ success: false, error: error.error, accounts: [] })
        }
      })
      const message: GetAccountsCommand = { command: TrezorCommand.GetAccounts,
        id: messageId, paths: requestedPaths, origin: window.origin }
      await this.postMessage(message)
    })
  }

  private _createBridge = () => {
    return new Promise((resolve) => {
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

  private _getPathForIndex = (index: number, scheme: string) => {
    if (scheme === TrezorDerivationPaths.Default) {
      return `m/44'/60'/0'/${index}`
    } else {
      throw Error(getLocale('braveWalletDeviceUnknownScheme'))
    }
  }

  private _getAccounts = async (from: number, to: number, scheme: string) => {
    const paths = []
    for (let i = from; i <= to; i++) {
      paths.push(this._getPathForIndex(i, scheme))
    }

    const accounts = await this.getTrezorAccounts(paths)
    if (!accounts.success) {
      throw Error(accounts.error)
    }
    return accounts.accounts
  }
}
