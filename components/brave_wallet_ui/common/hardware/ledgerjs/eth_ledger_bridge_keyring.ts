/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '../../../../common/locale'
import { LedgerEthereumKeyring } from '../interfaces'
import {
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult,
  HardwareImportScheme,
  AccountFromDevice
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  LedgerError,
  EthGetAccountResponse,
  EthGetAccountResponsePayload,
  EthSignTransactionResponse,
  EthSignTransactionResponsePayload,
  EthSignPersonalMessageResponse,
  EthSignPersonalMessageResponsePayload,
  EthSignEip712MessageResponse,
  EthSignEip712MessageResponsePayload
} from './ledger-messages'

import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class EthereumLedgerBridgeKeyring
  extends LedgerBridgeKeyring
  implements LedgerEthereumKeyring
{
  constructor(onAuthorized?: () => void) {
    super(onAuthorized)
  }

  bridgeType = (): BridgeType => {
    return BridgeTypes.EthLedger
  }

  getAccounts = async (
    from: number,
    count: number,
    scheme: HardwareImportScheme
  ): Promise<GetAccountsHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    const paths = []
    for (let i = 0; i < count; i++) {
      paths.push(scheme.pathTemplate(from + i))
    }
    return this.getAccountsFromDevice(paths)
  }

  signTransaction = async (
    path: string,
    rawTxHex: string
  ): Promise<SignHardwareOperationResult> => {
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
    if (
      data === LedgerBridgeErrorCodes.BridgeNotReady ||
      data === LedgerBridgeErrorCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      const ledgerError = data.payload as LedgerError
      return {
        success: false,
        error: ledgerError.message,
        code: ledgerError.statusCode
      }
    }

    const responsePayload = data.payload as EthSignTransactionResponsePayload
    return {
      success: true,
      payload: {
        v: responsePayload.v,
        r: responsePayload.r,
        s: responsePayload.s
      }
    }
  }

  signPersonalMessage = async (
    path: string,
    message: string
  ): Promise<SignHardwareOperationResult> => {
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
    if (
      data === LedgerBridgeErrorCodes.BridgeNotReady ||
      data === LedgerBridgeErrorCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      const ledgerError = data.payload as LedgerError
      return {
        success: false,
        error: ledgerError.message,
        code: ledgerError.statusCode
      }
    }

    const responsePayload =
      data.payload as EthSignPersonalMessageResponsePayload
    const signature = this.createMessageSignature(responsePayload)
    if (!signature) {
      return {
        success: false,
        error: getLocale('braveWalletLedgerValidationError')
      }
    }

    return { success: true, payload: signature }
  }

  signEip712Message = async (
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string
  ): Promise<SignHardwareOperationResult> => {
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
    if (
      data === LedgerBridgeErrorCodes.BridgeNotReady ||
      data === LedgerBridgeErrorCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      const ledgerError = data.payload as LedgerError
      return {
        success: false,
        error: ledgerError.message,
        code: ledgerError.statusCode
      }
    }

    const responsePayload = data.payload as EthSignEip712MessageResponsePayload
    const signature = this.createMessageSignature(responsePayload)
    if (!signature) {
      return {
        success: false,
        error: getLocale('braveWalletLedgerValidationError')
      }
    }

    return { success: true, payload: signature }
  }

  private readonly createMessageSignature = (
    result:
      | EthSignPersonalMessageResponsePayload
      | EthSignPersonalMessageResponsePayload
  ) => {
    // Convert the recovery identifier to standard ECDSA if using bitcoin
    // secp256k1 convention.
    let v = result.v < 27 ? result.v.toString(16) : (result.v - 27).toString(16)

    // Pad v with a leading zero if value is under `16` (i.e., single character
    // hex).
    if (v.length < 2) {
      v = `0${v}`
    }
    const signature = `0x${result.r}${result.s}${v}`
    return signature
  }

  private readonly getAccountsFromDevice = async (
    paths: string[]
  ): Promise<GetAccountsHardwareOperationResult> => {
    let accounts: AccountFromDevice[] = []
    for (const path of paths) {
      const data = await this.sendCommand<EthGetAccountResponse>({
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        path: path,
        origin: window.origin
      })
      if (
        data === LedgerBridgeErrorCodes.BridgeNotReady ||
        data === LedgerBridgeErrorCodes.CommandInProgress
      ) {
        return this.createErrorFromCode(data)
      }

      if (!data.payload.success) {
        const ledgerError = data.payload as LedgerError
        return {
          success: false,
          error: ledgerError,
          code: ledgerError.statusCode
        }
      }
      const responsePayload = data.payload as EthGetAccountResponsePayload

      accounts.push({
        address: responsePayload.address,
        derivationPath: path
      })
    }
    return { success: true, payload: accounts }
  }
}
