/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LedgerEthereumKeyring } from '../interfaces'
import {
  HardwareOperationResultAccounts,
  HardwareImportScheme,
  AccountFromDevice,
  fromUntrustedEthereumSignatureVRS,
  fromUntrustedEthereumSignatureBytes,
  HardwareOperationResultEthereumSignatureVRS,
  HardwareOperationResultEthereumSignatureBytes
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  EthGetAccountResponse,
  EthSignTransactionResponse,
  EthSignPersonalMessageResponse,
  EthSignEip712MessageResponse
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
  ): Promise<HardwareOperationResultAccounts> => {
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
  ): Promise<HardwareOperationResultEthereumSignatureVRS> => {
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
      return { ...data.payload }
    }

    const ethereumSignatureVRS = fromUntrustedEthereumSignatureVRS(
      data.payload.signature
    )
    if (!ethereumSignatureVRS) {
      return {
        success: false,
        error: 'Invalid signature',
        code: undefined
      }
    }

    return {
      success: true,
      signature: ethereumSignatureVRS
    }
  }

  signPersonalMessage = async (
    path: string,
    message: string
  ): Promise<HardwareOperationResultEthereumSignatureBytes> => {
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
      return { ...data.payload }
    }

    const ethereumSignatureBytes = fromUntrustedEthereumSignatureBytes(
      data.payload.signature
    )
    if (!ethereumSignatureBytes) {
      return {
        success: false,
        error: 'Invalid signature',
        code: undefined
      }
    }

    return {
      success: true,
      signature: ethereumSignatureBytes
    }
  }

  signEip712Message = async (
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string
  ): Promise<HardwareOperationResultEthereumSignatureBytes> => {
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
      return { ...data.payload }
    }

    const ethereumSignatureBytes = fromUntrustedEthereumSignatureBytes(
      data.payload.signature
    )
    if (!ethereumSignatureBytes) {
      return {
        success: false,
        error: 'Invalid signature',
        code: undefined
      }
    }

    return {
      success: true,
      signature: ethereumSignatureBytes
    }
  }

  private readonly getAccountsFromDevice = async (
    paths: string[]
  ): Promise<HardwareOperationResultAccounts> => {
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
        return { ...data.payload }
      }
      const responsePayload = data.payload

      accounts.push({
        address: responsePayload.address,
        derivationPath: path
      })
    }
    return {
      success: true,
      accounts: accounts
    }
  }
}
