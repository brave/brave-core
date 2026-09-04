/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import { BraveWallet } from '$wallet/constants/types'
import {
  AccountFromDevice,
  HardwareImportScheme,
  HardwareOperationResult,
  HardwareOperationError,
  HardwareOperationResultAccounts,
  HardwareOperationResultEthereumSignatureBytes,
  HardwareOperationResultEthereumSignatureVRS,
  HardwareOperationResultDeviceName,
} from '$wallet/common/hardware/types'
import * as TrezorMojom from 'gen/brave/components/brave_wallet/common/trezor_bridge.mojom.m.js' //
import { trezorBridgeRegistry } from './trezor_bridge_registry'
import TrezorBridgeKeyring from './trezor_bridge_keyring'

function getBridge(): TrezorMojom.TrezorBridgeRemote {
  return trezorBridgeRegistry.getBridge()
}

function toHardwareError(
  error: TrezorMojom.TrezorError,
): HardwareOperationError {
  return { success: false, error: error.message, code: error.code ?? undefined }
}

function prepareMojoTransactionPayload(
  ethTxData1559: BraveWallet.TxData1559,
  chainId: string,
): TrezorMojom.TrezorEthTransaction {
  const isEIP1559Transaction =
    ethTxData1559?.maxPriorityFeePerGas !== ''
    && ethTxData1559?.maxFeePerGas !== ''
  const data = [...Buffer.from(ethTxData1559.baseData.data ?? [])]
  if (isEIP1559Transaction) {
    return {
      legacy: undefined,
      eip1559: {
        to: ethTxData1559.baseData.to ?? '',
        value: ethTxData1559.baseData.value ?? '',
        data,
        chainId: parseInt(chainId, 16),
        nonce: ethTxData1559.baseData.nonce ?? '',
        gasLimit: ethTxData1559.baseData.gasLimit ?? '',
        maxFeePerGas: ethTxData1559.maxFeePerGas ?? '',
        maxPriorityFeePerGas: ethTxData1559.maxPriorityFeePerGas ?? '',
      },
    }
  }
  return {
    eip1559: undefined,
    legacy: {
      to: ethTxData1559.baseData.to ?? '',
      value: ethTxData1559.baseData.value ?? '',
      data,
      chainId: parseInt(chainId, 16),
      nonce: ethTxData1559.baseData.nonce ?? '',
      gasLimit: ethTxData1559.baseData.gasLimit ?? '',
      gasPrice: ethTxData1559.baseData.gasPrice ?? '',
    },
  }
}

export default class TrezorMojoBridgeKeyring extends TrezorBridgeKeyring {
  cancelOperation = async () => {}

  unlock = async (): Promise<HardwareOperationResult> => {
    const bridge = getBridge()
    const { error } = await bridge.unlock()
    return error ? toHardwareError(error) : { success: true }
  }

  getDeviceName = async (): Promise<HardwareOperationResultDeviceName> => {
    const bridge = getBridge()
    const { result } = await bridge.getDeviceName()
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, deviceName: result.deviceName ?? '' }
  }

  getAccounts = async (
    from: number,
    count: number,
    scheme: HardwareImportScheme,
  ): Promise<HardwareOperationResultAccounts> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const paths: string[] = []
    for (let i = 0; i < count; i++) {
      paths.push(scheme.pathTemplate(from + i))
    }
    const bridge = getBridge()
    const { result } = await bridge.getAccounts(paths)
    if (result.error) {
      return toHardwareError(result.error)
    }
    const accounts: AccountFromDevice[] = result.accounts!.map((account) => ({
      address: this.publicKeyToAddress(account.publicKey),
      derivationPath: account.serializedPath,
    }))
    const deviceName = await this.getDeviceName()
    return {
      success: true,
      accounts,
      deviceName: deviceName.success ? deviceName.deviceName : '',
    }
  }

  signTransaction = async (
    path: string,
    txid: string,
    ethTxData1559: BraveWallet.TxData1559,
    chainId: string,
  ): Promise<HardwareOperationResultEthereumSignatureVRS> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.signTransaction(
      path,
      prepareMojoTransactionPayload(ethTxData1559, chainId),
    )
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }

  signPersonalMessage = async (
    path: string,
    message: string,
  ): Promise<HardwareOperationResultEthereumSignatureBytes> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.signMessage(path, message)
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }

  signEip712Message = async (
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string,
    messageJson: string,
    domainJson: string,
    typesJson: string,
    primaryType: string,
  ): Promise<HardwareOperationResultEthereumSignatureBytes> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.signTypedMessage(
      path,
      domainSeparatorHex,
      hashStructMessageHex,
      typesJson,
      primaryType,
      domainJson,
      messageJson,
    )
    if (result.error) {
      if (result.error.code === 'Method_InvalidParameter') {
        return {
          success: false,
          error: getLocale('braveWalletTrezorSignTypedDataError'),
          code: undefined,
        }
      }
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }
}
