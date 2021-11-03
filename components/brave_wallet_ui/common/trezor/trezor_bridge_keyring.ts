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
  UnlockCommand,
  GetAccountsCommand,
  UnlockResponse,
  GetAccountsResponsePayload,
  TrezorAccount,

  TrezorError,
  TrezorBridgeTransport
} from '../../common/trezor/trezor-messages'
import { getLocale } from '../../../common/locale'
import { hardwareDeviceIdFromAddress } from '../async/lib'

export default class TrezorBridgeKeyring extends EventEmitter {
  constructor () {
    super()
    this.unlocked_ = false
    this.transport_ = new TrezorBridgeTransport()
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
    const paths = []

    const addZeroPath = (from > 0 || to < 0)
    if (addZeroPath) {
      // Add zero address to calculate device id.
      paths.push(this.getPathForIndex(0, TrezorDerivationPaths.Default))
    }
    for (let i = from; i <= to; i++) {
      paths.push(this.getPathForIndex(i, scheme))
    }
    const accounts = await this.getAccountsFromDevice(paths, addZeroPath)
    if (!accounts.success) {
      throw Error(accounts.error)
    }
    return accounts.accounts
  }

  isUnlocked = () => {
    return this.unlocked_
  }

  unlock = () => {
    return new Promise(async (resolve, reject) => {
      // @ts-ignore
      const messageId: string = crypto.randomUUID()
      this.getTransport().addEventListener(messageId, (data: UnlockResponse) => {
        this.unlocked_ = data.result
        if (data.result) {
          resolve(true)
        } else {
          reject(false)
        }
      })
      const message: UnlockCommand = { id: messageId, origin: window.origin,
        command: TrezorCommand.Unlock }
      await this.getTransport().postMessage(message)
    })
  }

  getTransport = () => {
    return this.transport_
  }

  private getHashFromAddress = async (address: string) => {
    return hardwareDeviceIdFromAddress(address)
  }

  private getDeviceIdFromAccountsList = async (accountsList: TrezorAccount[]) => {
    const zeroPath = this.getPathForIndex(0, TrezorDerivationPaths.Default)
    for (const value of accountsList) {
      if (value.serializedPath !== zeroPath) {
        continue
      }
      const address = this.publicKeyToAddress(value.publicKey)
      return this.getHashFromAddress(address)
    }
    return ''
  }

  private publicKeyToAddress = (key: string) => {
    const buffer = Buffer.from(key, 'hex')
    const address = publicToAddress(buffer, true).toString('hex')
    return toChecksumAddress(`0x${address}`)
  }

  private getAccountsFromDevice = async (paths: string[], skipZeroPath: Boolean): Promise<TrezorBridgeAccountsPayload> => {
    return new Promise(async (resolve, reject) => {
      const requestedPaths = []
      for (const path of paths) {
        requestedPaths.push({ path: path })
      }
      // @ts-ignore
      const messageId: string = crypto.randomUUID()
      this.getTransport().addEventListener(messageId, async (data: GetAccountsResponsePayload) => {
        if (data.payload.success) {
          let accounts = []
          const accountsList = data.payload.payload as TrezorAccount[]
          this.deviceId_ = await this.getDeviceIdFromAccountsList(accountsList)
          const zeroPath = this.getPathForIndex(0, TrezorDerivationPaths.Default)
          for (const value of accountsList) {
            // If requested addresses do not have zero indexed adress we add it
            // intentionally to calculate device id and should not add it to
            // returned accounts
            if (skipZeroPath && (value.serializedPath === zeroPath)) {
              continue
            }
            accounts.push({
              address: this.publicKeyToAddress(value.publicKey),
              derivationPath: value.serializedPath,
              name: this.type(),
              hardwareVendor: this.type(),
              deviceId: this.deviceId_
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
      await this.getTransport().postMessage(message)
    })
  }

  private getPathForIndex = (index: number, scheme: string) => {
    if (scheme === TrezorDerivationPaths.Default) {
      return `m/44'/60'/0'/${index}`
    } else {
      throw Error(getLocale('braveWalletDeviceUnknownScheme'))
    }
  }

}
