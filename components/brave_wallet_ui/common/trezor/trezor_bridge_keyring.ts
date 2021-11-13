/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

const { EventEmitter } = require('events')
import { publicToAddress, toChecksumAddress, bufferToHex } from 'ethereumjs-util'
import {
  TrezorDerivationPaths, TrezorBridgeAccountsPayload
} from '../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import {
  kTrezorHardwareVendor,
  TransactionInfo
} from '../../constants/types'
import {
  TrezorCommand,
  UnlockResponse,
  GetAccountsResponsePayload,
  TrezorAccount,
  SignTransactionCommandPayload,
  TrezorFrameCommand,
  SignTransactionResponsePayload,
  SignMessageCommandPayload,
  SignMessageResponsePayload
} from '../../common/trezor/trezor-messages'
import { sendTrezorCommand } from '../../common/trezor/trezor-bridge-transport'
import { getLocale } from '../../../common/locale'
import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'
import { SignHardwareMessageOperationResult, SignHardwareTransactionOperationResult } from '../../common/hardware_operations'

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

  signTransaction = async (path: string, txInfo: TransactionInfo, chainId: string): Promise<SignHardwareTransactionOperationResult> => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return { success: false, error: getLocale('braveWalletUnlockError') }
    }
    const data = await this.sendTrezorCommand<SignTransactionResponsePayload>({
      command: TrezorCommand.SignTransaction,
      // @ts-expect-error
      id: crypto.randomUUID(),
      payload: this.prepareTransactionPayload(path, txInfo, chainId),
      origin: window.origin
    })
    if (!data || !data.payload) {
      return { success: false, error: getLocale('braveWalletProcessTransactionError') }
    }
    if (!data.payload.success) {
      return { success: false, error: data.payload.payload.error, code: data.payload.payload.code }
    }
    return { success: true, payload: data.payload.payload }
  }

  signPersonalMessage = async (path: string, message: string): Promise<SignHardwareMessageOperationResult> => {
    if (!this.isUnlocked() && !(await this.unlock())) {
      return { success: false, error: getLocale('braveWalletUnlockError') }
    }
    const data = await this.sendTrezorCommand<SignMessageResponsePayload>({
      command: TrezorCommand.SignMessage,
      // @ts-expect-error
      id: crypto.randomUUID(),
      payload: this.prepareSignMessagePayload(path, message),
      origin: window.origin
    })
    if (!data) {
      return { success: false, error: getLocale('braveWalletProcessMessageError') }
    }
    if (!data.payload.success) {
      const unsuccess = data.payload
      return { success: false, error: unsuccess.payload.error, code: unsuccess.payload.code }
    }
    return { success: true, payload: data.payload.payload.signature }
  }

  isUnlocked = () => {
    return this.unlocked_
  }

  unlock = async () => {
    const data = await this.sendTrezorCommand<UnlockResponse>({
      // @ts-ignore
      id: crypto.randomUUID(),
      origin: window.origin,
      command: TrezorCommand.Unlock })
    if (!data) {
      return false
    }
    this.unlocked_ = data.result
    if (data.result) {
      return true
    }
    return false
  }

  private async sendTrezorCommand<T> (command: TrezorFrameCommand): Promise<T | false> {
    return sendTrezorCommand<T>(command)
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

  private prepareTransactionPayload = (path: string, txInfo: TransactionInfo, chainId: string): SignTransactionCommandPayload => {
    const isEIP1559Transaction = txInfo.txData.maxPriorityFeePerGas !== '' && txInfo.txData.maxFeePerGas !== ''
    if (isEIP1559Transaction) {
      return this.createEIP1559TransactionPayload(path, txInfo, chainId)
    }
    return this.createLegacyTransactionPayload(path, txInfo, chainId)
  }

  private createEIP1559TransactionPayload = (path: string, txInfo: TransactionInfo, chainId: string): SignTransactionCommandPayload => {
    return {
      path: path,
      transaction: {
        to: txInfo.txData.baseData.to,
        value: txInfo.txData.baseData.value,
        data: bufferToHex(Buffer.from(txInfo.txData.baseData.data)).toString(),
        chainId: parseInt(chainId, 16),
        nonce: txInfo.txData.baseData.nonce,
        gasLimit: txInfo.txData.baseData.gasLimit,
        maxFeePerGas: txInfo.txData.maxFeePerGas,
        maxPriorityFeePerGas: txInfo.txData.maxPriorityFeePerGas
      }
    }
  }

  private createLegacyTransactionPayload = (path: string, txInfo: TransactionInfo, chainId: string): SignTransactionCommandPayload => {
    return {
      path: path,
      transaction: {
        to: txInfo.txData.baseData.to,
        value: txInfo.txData.baseData.value,
        data: bufferToHex(Buffer.from(txInfo.txData.baseData.data)).toString(),
        chainId: parseInt(chainId, 16),
        nonce: txInfo.txData.baseData.nonce,
        gasLimit: txInfo.txData.baseData.gasLimit,
        gasPrice: txInfo.txData.baseData.gasPrice
      }
    }
  }

  private readonly prepareSignMessagePayload = (path: string, message: string): SignMessageCommandPayload => {
    return { path: path, message: message }
  }

  private readonly publicKeyToAddress = (key: string) => {
    const buffer = Buffer.from(key, 'hex')
    const address = publicToAddress(buffer, true).toString('hex')
    return toChecksumAddress(`0x${address}`)
  }

  private getAccountsFromDevice = async (paths: string[], skipZeroPath: Boolean): Promise<TrezorBridgeAccountsPayload> => {
    const requestedPaths = []
    for (const path of paths) {
      requestedPaths.push({ path: path })
    }
    const data = await this.sendTrezorCommand<GetAccountsResponsePayload>({
      command: TrezorCommand.GetAccounts,
      // @ts-ignore
      id: crypto.randomUUID(),
      paths: requestedPaths,
      origin: window.origin })
    if (!data || !data.payload.success) {
      return { success: false, error: getLocale('braveWalletCreateBridgeError'), accounts: [] }
    }

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
    return { success: true, accounts: [...accounts] }
  }

  private getPathForIndex = (index: number, scheme: string) => {
    if (scheme === TrezorDerivationPaths.Default) {
      return `m/44'/60'/0'/0/${index}`
    } else {
      throw Error(getLocale('braveWalletDeviceUnknownScheme'))
    }
  }
}
