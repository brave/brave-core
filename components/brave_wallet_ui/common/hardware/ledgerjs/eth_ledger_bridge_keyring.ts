/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from 'chrome://resources/js/assert_ts.js'
import { BraveWallet } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'
import { LedgerEthereumKeyring } from '../interfaces'
import {
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult,
  LedgerDerivationPaths
} from '../types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  LedgerError
} from './ledger-messages'
import {
  EthGetAccountResponse,
  EthGetAccountResponsePayload,
  EthSignTransactionResponse,
  EthSignTransactionResponsePayload,
  EthSignPersonalMessageResponse,
  EthSignPersonalMessageResponsePayload,
  EthSignEip712MessageResponse,
  EthSignEip712MessageResponsePayload
} from './eth-ledger-messages'

import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'
import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class EthereumLedgerBridgeKeyring extends LedgerBridgeKeyring implements LedgerEthereumKeyring {
  constructor (onAuthorized?: () => void) {
    super(onAuthorized)
  }

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.ETH
  }

  getAccounts = async (from: number, to: number, scheme: string): Promise<GetAccountsHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    from = (from >= 0) ? from : 0
    const paths = []
    const addZeroPath = (from > 0 || to < 0)
    if (addZeroPath) {
      // Add zero address to calculate device id.
      paths.push(this.getPathForIndex(0, scheme))
    }
    for (let i = from; i <= to; i++) {
      paths.push(this.getPathForIndex(i, scheme))
    }
    return this.getAccountsFromDevice(paths, addZeroPath, scheme)
  }

  signTransaction = async (path: string, rawTxHex: string): Promise<SignHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    const data = await this.sendCommand<EthSignTransactionResponse>({
      command: LedgerCommand.SignTransaction,
      id: LedgerCommand.SignTransaction,
      path: path,
      rawTxHex: rawTxHex,
      origin: window.origin
    })
    if (data === LedgerBridgeErrorCodes.BridgeNotReady ||
        data === LedgerBridgeErrorCodes.CommandInProgress) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      const ledgerError = data.payload as LedgerError
      return { success: false, error: ledgerError.message, code: ledgerError.statusCode }
    }

    const responsePayload = data.payload as EthSignTransactionResponsePayload
    return { success: true, payload: { v: responsePayload.v, r: responsePayload.r, s: responsePayload.s } }
  }

  signPersonalMessage = async (path: string, message: string): Promise<SignHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    const messageHex = Buffer.from(message).toString('hex')
    const data = await this.sendCommand<EthSignPersonalMessageResponse>({
      command: LedgerCommand.SignPersonalMessage,
      id: LedgerCommand.SignPersonalMessage,
      path: path,
      origin: window.origin,
      messageHex: messageHex
    })
    if (data === LedgerBridgeErrorCodes.BridgeNotReady ||
        data === LedgerBridgeErrorCodes.CommandInProgress) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      const ledgerError = data.payload as LedgerError
      return { success: false, error: ledgerError.message, code: ledgerError.statusCode }
    }

    const responsePayload = data.payload as EthSignPersonalMessageResponsePayload
    const signature = this.createMessageSignature(responsePayload)
    if (!signature) {
      return { success: false, error: getLocale('braveWalletLedgerValidationError') }
    }

    return { success: true, payload: signature }
  }

  signEip712Message = async (path: string, domainSeparatorHex: string, hashStructMessageHex: string): Promise<SignHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    const data = await this.sendCommand<EthSignEip712MessageResponse>({
      command: LedgerCommand.SignEip712Message,
      id: LedgerCommand.SignEip712Message,
      path: path,
      origin: window.origin,
      domainSeparatorHex: domainSeparatorHex,
      hashStructMessageHex: hashStructMessageHex
    })
    if (data === LedgerBridgeErrorCodes.BridgeNotReady ||
        data === LedgerBridgeErrorCodes.CommandInProgress) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      const ledgerError = data.payload as LedgerError
      return { success: false, error: ledgerError.message, code: ledgerError.statusCode }
    }

    const responsePayload = data.payload as EthSignEip712MessageResponsePayload
    const signature = this.createMessageSignature(responsePayload)
    if (!signature) {
      return { success: false, error: getLocale('braveWalletLedgerValidationError') }
    }

    return { success: true, payload: signature }
  }

  private readonly createMessageSignature = (result: EthSignPersonalMessageResponsePayload | EthSignPersonalMessageResponsePayload) => {
    // Convert the recovery identifier to standard ECDSA if using bitcoin secp256k1 convention.
    let v = result.v < 27
      ? result.v.toString(16)
      : (result.v - 27).toString(16)

    // Pad v with a leading zero if value is under `16` (i.e., single character hex).
    if (v.length < 2) {
      v = `0${v}`
    }
    const signature = `0x${result.r}${result.s}${v}`
    return signature
  }

  private readonly getAccountsFromDevice = async (paths: string[], skipZeroPath: boolean, scheme: string): Promise<GetAccountsHardwareOperationResult> => {
    let accounts = []
    const zeroPath = this.getPathForIndex(0, scheme)
    for (const path of paths) {
      const data = await this.sendCommand<EthGetAccountResponse>({
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        path: path,
        origin: window.origin
      })
      if (data === LedgerBridgeErrorCodes.BridgeNotReady ||
          data === LedgerBridgeErrorCodes.CommandInProgress) {
        return this.createErrorFromCode(data)
      }

      if (!data.payload.success) {
        const ledgerError = data.payload as LedgerError
        return { success: false, error: ledgerError, code: ledgerError.statusCode }
      }
      const responsePayload = data.payload as EthGetAccountResponsePayload

      if (path === zeroPath) {
        this.deviceId = await hardwareDeviceIdFromAddress(responsePayload.address)
        if (skipZeroPath) {
          // If requested addresses do not have zero indexed adress we add it
          // intentionally to calculate device id and should not add it to
          // returned accounts
          continue
        }
      }

      accounts.push({
        address: responsePayload.address,
        derivationPath: path,
        name: this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId,
        coin: this.coin(),
        network: undefined
      })
    }
    return { success: true, payload: accounts }
  }

  private readonly getPathForIndex = (index: number, scheme: string): string => {
    if (scheme === LedgerDerivationPaths.LedgerLive) {
      return `m/44'/60'/${index}'/0/0`
    }
    if (scheme === LedgerDerivationPaths.Deprecated) {
      return `m/44'/60'/${index}'/0`
    }

    assert(scheme === LedgerDerivationPaths.Legacy, '')
    return `m/44'/60'/0'/${index}`
  }
}
