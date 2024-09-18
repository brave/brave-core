/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  publicToAddress,
  toChecksumAddress,
  bufferToHex
} from 'ethereumjs-util'
import { BraveWallet } from '../../../constants/types'
import { getLocale } from '../../../../common/locale'
import {
  TrezorCommand,
  UnlockResponsePayload,
  GetAccountsResponsePayload,
  SignTransactionCommandPayload,
  TrezorFrameCommand,
  SignTransactionResponsePayload,
  SignMessageCommandPayload,
  SignMessageResponsePayload,
  TrezorErrorsCodes,
  SignTransactionResponse,
  SignMessageResponse,
  TrezorGetAccountsResponse,
  SignTypedMessageResponsePayload
} from './trezor-messages'
import { sendTrezorCommand, closeTrezorBridge } from './trezor-bridge-transport'
import {
  AccountFromDevice,
  HardwareImportScheme,
  HardwareOperationResultAccounts,
  HardwareOperationResult,
  HardwareOperationError,
  HardwareOperationResultEthereumSignatureBytes,
  HardwareOperationResultEthereumSignatureVRS,
  fromUntrustedEthereumSignatureBytes,
  fromUntrustedEthereumSignatureVRS
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import { Unsuccessful } from './trezor-connect-types'
import { TrezorKeyring } from '../interfaces'

export default class TrezorBridgeKeyring implements TrezorKeyring {
  private unlocked: boolean = false

  bridgeType = (): BridgeType => {
    return BridgeTypes.EthTrezor
  }

  isUnlocked = (): boolean => {
    return this.unlocked
  }

  cancelOperation = async () => {
    closeTrezorBridge()
  }

  unlock = async (): Promise<HardwareOperationResult> => {
    const data = await this.sendTrezorCommand<UnlockResponsePayload>({
      id: TrezorCommand.Unlock,
      origin: window.origin,
      command: TrezorCommand.Unlock
    })
    if (
      data === TrezorErrorsCodes.BridgeNotReady ||
      data === TrezorErrorsCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    this.unlocked = data.payload.success
    if (!data.payload.success) {
      const response: Unsuccessful = data.payload as Unsuccessful
      const error =
        response.payload?.error ?? getLocale('braveWalletUnlockError')
      const code = response.payload?.code ?? ''
      return { success: false, error: error, code: code }
    }
    return { success: true }
  }

  getAccounts = async (
    from: number,
    count: number,
    scheme: HardwareImportScheme
  ): Promise<HardwareOperationResultAccounts> => {
    if (!this.isUnlocked()) {
      const unlocked = await this.unlock()
      if (!unlocked.success) {
        return unlocked
      }
    }
    const paths = []
    for (let i = 0; i < count; i++) {
      paths.push(scheme.pathTemplate(from + i))
    }
    return this.getAccountsFromDevice(paths)
  }

  signTransaction = async (
    path: string,
    txid: string,
    ethTxData1559: BraveWallet.TxData1559,
    chainId: string
  ): Promise<HardwareOperationResultEthereumSignatureVRS> => {
    if (!this.isUnlocked()) {
      const unlocked = await this.unlock()
      if (!unlocked.success) {
        return unlocked
      }
    }
    // TODO(apaymyshev): why we are sending tx meta id to trezor frame?
    const data = await this.sendTrezorCommand<SignTransactionResponsePayload>({
      command: TrezorCommand.SignTransaction,
      id: txid,
      payload: this.prepareTransactionPayload(path, ethTxData1559, chainId),
      origin: window.origin
    })
    if (
      data === TrezorErrorsCodes.BridgeNotReady ||
      data === TrezorErrorsCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    const response: SignTransactionResponse = data.payload
    if (!response.success) {
      return {
        success: false,
        error: response.payload.error,
        code: response.payload.code
      }
    }
    const ethereumSignatureVRS = fromUntrustedEthereumSignatureVRS(
      response.payload
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
    if (!this.isUnlocked()) {
      const unlocked = await this.unlock()
      if (!unlocked.success) {
        return unlocked
      }
    }
    const data = await this.sendTrezorCommand<SignMessageResponsePayload>({
      command: TrezorCommand.SignMessage,
      id: path,
      payload: this.prepareSignMessagePayload(path, message),
      origin: window.origin
    })
    if (
      data === TrezorErrorsCodes.BridgeNotReady ||
      data === TrezorErrorsCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    const response: SignMessageResponse = data.payload
    if (!response.success) {
      const unsuccess = response.payload
      return { success: false, error: unsuccess.error, code: unsuccess.code }
    }
    const ethereumSignatureBytes = fromUntrustedEthereumSignatureBytes(
      response.payload
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
    if (!this.isUnlocked()) {
      const unlocked = await this.unlock()
      if (!unlocked.success) {
        return unlocked
      }
    }
    const data = await this.sendTrezorCommand<SignTypedMessageResponsePayload>({
      command: TrezorCommand.SignTypedMessage,
      id: path,
      payload: {
        path: path,
        domain_separator_hash: domainSeparatorHex,
        message_hash: hashStructMessageHex,
        metamask_v4_compat: true,
        data: {
          types: { EIP712Domain: [] },
          primaryType: 'UnknownType',
          domain: {},
          message: {}
        }
      },
      origin: window.origin
    })

    if (
      data === TrezorErrorsCodes.BridgeNotReady ||
      data === TrezorErrorsCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    const response: SignMessageResponse = data.payload
    if (!response.success) {
      const unsuccess = response.payload
      if (unsuccess.code && unsuccess.code === 'Method_InvalidParameter') {
        return {
          success: false,
          error: getLocale('braveWalletTrezorSignTypedDataError'),
          code: undefined
        }
      }
      return { success: false, error: unsuccess.error, code: unsuccess.code }
    }

    const ethereumSignatureBytes = fromUntrustedEthereumSignatureBytes(
      response.payload
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

  private async sendTrezorCommand<T>(
    command: TrezorFrameCommand
  ): Promise<T | TrezorErrorsCodes> {
    return sendTrezorCommand<T>(command)
  }

  private prepareTransactionPayload = (
    path: string,
    ethTxData1559: BraveWallet.TxData1559,
    chainId: string
  ): SignTransactionCommandPayload => {
    const isEIP1559Transaction =
      ethTxData1559?.maxPriorityFeePerGas !== '' &&
      ethTxData1559?.maxFeePerGas !== ''
    if (isEIP1559Transaction) {
      return this.createEIP1559TransactionPayload(path, ethTxData1559, chainId)
    }
    return this.createLegacyTransactionPayload(path, ethTxData1559, chainId)
  }

  private createEIP1559TransactionPayload = (
    path: string,
    ethTxData1559: BraveWallet.TxData1559,
    chainId: string
  ): SignTransactionCommandPayload => {
    return {
      path: path,
      transaction: {
        to: ethTxData1559.baseData.to ?? '',
        value: ethTxData1559.baseData.value ?? '',
        data: bufferToHex(
          Buffer.from(ethTxData1559.baseData.data ?? [])
        ).toString(),
        chainId: parseInt(chainId, 16),
        nonce: ethTxData1559.baseData.nonce ?? '',
        gasLimit: ethTxData1559.baseData.gasLimit ?? '',
        maxFeePerGas: ethTxData1559.maxFeePerGas ?? '',
        maxPriorityFeePerGas: ethTxData1559.maxPriorityFeePerGas ?? ''
      }
    }
  }

  private createLegacyTransactionPayload = (
    path: string,
    ethTxData1559: BraveWallet.TxData1559,
    chainId: string
  ): SignTransactionCommandPayload => {
    return {
      path: path,
      transaction: {
        to: ethTxData1559.baseData.to ?? '',
        value: ethTxData1559.baseData.value ?? '',
        data: bufferToHex(
          Buffer.from(ethTxData1559.baseData.data ?? [])
        ).toString(),
        chainId: parseInt(chainId, 16),
        nonce: ethTxData1559.baseData.nonce ?? '',
        gasLimit: ethTxData1559.baseData.gasLimit ?? '',
        gasPrice: ethTxData1559.baseData.gasPrice ?? ''
      }
    }
  }

  private readonly prepareSignMessagePayload = (
    path: string,
    message: string
  ): SignMessageCommandPayload => {
    return { path: path, message: message }
  }

  private readonly publicKeyToAddress = (key: string) => {
    const buffer = Buffer.from(key, 'hex')
    const address = publicToAddress(buffer, true).toString('hex')
    return toChecksumAddress(`0x${address}`)
  }

  private readonly getAccountsFromDevice = async (
    paths: string[]
  ): Promise<HardwareOperationResultAccounts> => {
    const requestedPaths = []
    for (const path of paths) {
      requestedPaths.push({ path: path })
    }
    const data = await this.sendTrezorCommand<GetAccountsResponsePayload>({
      command: TrezorCommand.GetAccounts,
      id: TrezorCommand.GetAccounts,
      paths: requestedPaths,
      origin: window.origin
    })
    if (
      data === TrezorErrorsCodes.BridgeNotReady ||
      data === TrezorErrorsCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }

    const response: TrezorGetAccountsResponse = data.payload
    if (!response.success) {
      const unsuccess = response.payload
      return { success: false, error: unsuccess.error, code: unsuccess.code }
    }

    let accounts: AccountFromDevice[] = []
    for (const value of response.payload) {
      accounts.push({
        address: this.publicKeyToAddress(value.publicKey),
        derivationPath: value.serializedPath
      })
    }
    return { success: true, accounts: [...accounts] }
  }

  private readonly createErrorFromCode = (
    code: TrezorErrorsCodes
  ): HardwareOperationError => {
    const deviceName = getLocale('braveWalletConnectHardwareTrezor')

    switch (code) {
      case TrezorErrorsCodes.BridgeNotReady:
        return {
          success: false,
          error: getLocale('braveWalletBridgeNotReady').replace(
            '$1',
            deviceName
          ),
          code: code
        }
      case TrezorErrorsCodes.CommandInProgress:
        return {
          success: false,
          error: getLocale('braveWalletBridgeCommandInProgress').replace(
            '$1',
            deviceName
          ),
          code: code
        }
    }
  }
}
