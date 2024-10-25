// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'
import {
  FetchHardwareWalletAccountsProps,
  HardwareOperationResultAccounts,
  HardwareOperationResult,
  HardwareOperationResultEthereumSignatureBytes,
  HardwareOperationResultSolanaSignature
} from '../hardware/types'
import { getLocale } from '../../../common/locale'
import type WalletApiProxy from '../../common/wallet_api_proxy'
import {
  getHardwareKeyring,
  getLedgerBitcoinHardwareKeyring,
  getLedgerEthereumHardwareKeyring,
  getLedgerFilecoinHardwareKeyring,
  getLedgerSolanaHardwareKeyring,
  getTrezorHardwareKeyring
} from '../api/hardware_keyrings'

import { TrezorErrorsCodes } from '../hardware/trezor/trezor-messages'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import EthereumLedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import {
  BraveWallet,
  HardwareWalletResponseCodeType,
  SerializableTransactionInfo
} from '../../constants/types'
import {
  LedgerBitcoinKeyring,
  LedgerEthereumKeyring,
  LedgerFilecoinKeyring,
  LedgerSolanaKeyring
} from '../hardware/interfaces'

export function dialogErrorFromLedgerErrorCode(
  code: string | number
): HardwareWalletResponseCodeType {
  if (code === 'TransportOpenUserCancelled') {
    return 'deviceNotConnected'
  }

  if (code === 'TransportLocked') {
    return 'deviceBusy'
  }

  // @ledgerhq/errors StatusCodes.CONDITIONS_OF_USE_NOT_SATISFIED
  const CONDITIONS_OF_USE_NOT_SATISFIED = 0x6985

  if (code === CONDITIONS_OF_USE_NOT_SATISFIED) {
    return 'transactionRejected'
  }

  return 'openLedgerApp'
}

export function dialogErrorFromTrezorErrorCode(
  code: TrezorErrorsCodes | string
): HardwareWalletResponseCodeType {
  if (code === TrezorErrorsCodes.CommandInProgress) {
    return 'deviceBusy'
  }
  if (code === 'Method_Interrupted') {
    return 'transactionRejected'
  }
  return 'openLedgerApp'
}

export async function signTrezorTransaction(
  apiProxy: WalletApiProxy,
  path: string,
  txInfo: Pick<SerializableTransactionInfo, 'id' | 'chainId' | 'txDataUnion'>,
  deviceKeyring: TrezorBridgeKeyring = getTrezorHardwareKeyring()
): Promise<HardwareOperationResult> {
  const nonce = await apiProxy.ethTxManagerProxy.getNonceForHardwareTransaction(
    txInfo.id
  )
  if (!nonce || !nonce.nonce) {
    return {
      success: false,
      error: getLocale('braveWalletApproveTransactionError'),
      code: undefined
    }
  }

  if (!txInfo.txDataUnion.ethTxData1559) {
    return {
      success: false,
      error: getLocale('braveWalletApproveTransactionError'),
      code: undefined
    }
  }

  const ethTxData1559 = structuredClone(txInfo.txDataUnion.ethTxData1559)
  ethTxData1559.baseData.nonce = nonce.nonce

  const signed = await deviceKeyring.signTransaction(
    path,
    txInfo.id,
    ethTxData1559,
    txInfo.chainId
  )
  if (!signed.success) {
    const error = signed.error
      ? signed.error
      : getLocale('braveWalletSignOnDeviceError')
    if (signed.code === TrezorErrorsCodes.CommandInProgress) {
      return {
        success: false,
        error: error,
        code: 'deviceBusy'
      }
    }
    return {
      success: false,
      error: error,
      code: signed.code
    }
  }
  const result = await apiProxy.ethTxManagerProxy.processEthHardwareSignature(
    txInfo.id,
    signed.signature
  )
  if (!result.status) {
    return {
      success: false,
      error: getLocale('braveWalletProcessTransactionError'),
      code: undefined
    }
  }
  return { success: true }
}

export async function signLedgerEthereumTransaction(
  apiProxy: WalletApiProxy,
  path: string,
  txMetaId: string,
  deviceKeyring: LedgerEthereumKeyring = getLedgerEthereumHardwareKeyring()
): Promise<HardwareOperationResult> {
  const nonce = await apiProxy.ethTxManagerProxy.getNonceForHardwareTransaction(
    txMetaId
  )
  if (!nonce || !nonce.nonce) {
    return {
      success: false,
      error: getLocale('braveWalletApproveTransactionError'),
      code: undefined
    }
  }
  const { hexMessage } =
    await apiProxy.ethTxManagerProxy.getEthTransactionMessageToSign(txMetaId)
  if (!hexMessage) {
    return {
      success: false,
      error: getLocale('braveWalletNoMessageToSignError'),
      code: undefined
    }
  }

  const signed = await deviceKeyring.signTransaction(path, hexMessage)

  if (!signed.success) {
    return {
      success: false,
      error: signed.error ?? getLocale('braveWalletSignOnDeviceError'),
      code: signed.code ?? ''
    }
  }
  const result = await apiProxy.ethTxManagerProxy.processEthHardwareSignature(
    txMetaId,
    signed.signature
  )
  if (!result || !result.status) {
    return {
      success: false,
      error: getLocale('braveWalletProcessTransactionError'),
      code: undefined
    }
  }
  return { success: true }
}

export async function signLedgerFilecoinTransaction(
  apiProxy: WalletApiProxy,
  txMetaId: string,
  deviceKeyring: LedgerFilecoinKeyring = getLedgerFilecoinHardwareKeyring()
): Promise<HardwareOperationResult> {
  const { jsonMessage } =
    await apiProxy.filTxManagerProxy.getFilTransactionMessageToSign(txMetaId)
  if (!jsonMessage) {
    return {
      success: false,
      error: getLocale('braveWalletNoMessageToSignError'),
      code: undefined
    }
  }

  const signed = await deviceKeyring.signTransaction(jsonMessage)
  if (!signed.success) {
    return {
      success: false,
      error: signed.error ?? getLocale('braveWalletSignOnDeviceError'),
      code: signed.code ?? ''
    }
  }

  const result = await apiProxy.filTxManagerProxy.processFilHardwareSignature(
    txMetaId,
    signed.signature
  )
  if (!result || !result.status) {
    return {
      success: false,
      error: getLocale('braveWalletProcessTransactionError'),
      code: undefined
    }
  }
  return { success: true }
}

export async function signLedgerSolanaTransaction(
  apiProxy: WalletApiProxy,
  path: string,
  txMetaId: string,
  deviceKeyring: LedgerSolanaKeyring = getLedgerSolanaHardwareKeyring()
): Promise<HardwareOperationResult> {
  const { message } =
    await apiProxy.solanaTxManagerProxy.getSolTransactionMessageToSign(txMetaId)
  if (!message) {
    return {
      success: false,
      error: getLocale('braveWalletNoMessageToSignError'),
      code: undefined
    }
  }
  const signed = await deviceKeyring.signTransaction(path, Buffer.from(message))
  if (!signed.success) {
    return {
      success: false,
      error: signed.error ?? getLocale('braveWalletSignOnDeviceError'),
      code: signed.code ?? ''
    }
  }

  const result =
    await apiProxy.solanaTxManagerProxy.processSolanaHardwareSignature(
      txMetaId,
      signed.signature
    )
  if (!result || !result.status) {
    return {
      success: false,
      error: getLocale('braveWalletProcessTransactionError'),
      code: undefined
    }
  }

  return { success: result.status }
}

export async function signLedgerBitcoinTransaction(
  apiProxy: WalletApiProxy,
  txMetaId: string,
  deviceKeyring: LedgerBitcoinKeyring = getLedgerBitcoinHardwareKeyring()
): Promise<HardwareOperationResult> {
  const { data } =
    await apiProxy.btcTxManagerProxy.getBtcHardwareTransactionSignData(txMetaId)
  if (!data) {
    return {
      success: false,
      error: getLocale('braveWalletNoMessageToSignError'),
      code: undefined
    }
  }
  const signed = await deviceKeyring.signTransaction(
    data.inputs.map((i) => {
      return {
        txBytes: Buffer.from(i.txBytes),
        outputIndex: i.outputIndex,
        associatedPath: i.associatedPath
      }
    }),
    Buffer.from(data.outputScript),
    data.changePath,
    data.lockTime
  )
  if (!signed.success) {
    return {
      success: false,
      error: signed.error ?? getLocale('braveWalletSignOnDeviceError'),
      code: signed.code ?? ''
    }
  }

  const result = await apiProxy.btcTxManagerProxy.processBtcHardwareSignature(
    txMetaId,
    signed.signature
  )
  if (!result || !result.status) {
    return {
      success: false,
      error: getLocale('braveWalletProcessTransactionError'),
      code: undefined
    }
  }

  return { success: result.status }
}

export async function signEthMessageWithHardwareKeyring(
  vendor: BraveWallet.HardwareVendor,
  path: string,
  messageData: Omit<BraveWallet.SignMessageRequest, 'originInfo'>
): Promise<HardwareOperationResultEthereumSignatureBytes> {
  const deviceKeyring = getHardwareKeyring(vendor, messageData.coin)
  const signTypedData = messageData.signData.ethSignTypedData
  const standardSignData = messageData.signData.ethStandardSignData
  if (deviceKeyring instanceof EthereumLedgerBridgeKeyring) {
    if (signTypedData) {
      if (!signTypedData.domainHash || !signTypedData.primaryHash) {
        return {
          success: false,
          error: getLocale('braveWalletUnknownInternalError'),
          code: undefined
        }
      }
      return deviceKeyring.signEip712Message(
        path,
        Buffer.from(signTypedData.domainHash).toString('hex'),
        Buffer.from(signTypedData.primaryHash).toString('hex')
      )
    }
    if (!standardSignData) {
      return {
        success: false,
        error: getLocale('braveWalletUnknownInternalError'),
        code: undefined
      }
    }
    return deviceKeyring.signPersonalMessage(path, standardSignData.message)
  } else if (deviceKeyring instanceof TrezorBridgeKeyring) {
    if (signTypedData) {
      if (!signTypedData.domainHash || !signTypedData.primaryHash) {
        return {
          success: false,
          error: getLocale('braveWalletUnknownInternalError'),
          code: undefined
        }
      }
      return deviceKeyring.signEip712Message(
        path,
        Buffer.from(signTypedData.domainHash).toString('hex'),
        Buffer.from(signTypedData.primaryHash).toString('hex'),
        signTypedData.messageJson,
        signTypedData.domainJson,
        signTypedData.typesJson,
        signTypedData.primaryType
      )
    }
    if (!standardSignData) {
      return {
        success: false,
        error: getLocale('braveWalletUnknownInternalError'),
        code: undefined
      }
    }
    return deviceKeyring.signPersonalMessage(path, standardSignData.message)
  } else if (deviceKeyring instanceof SolanaLedgerBridgeKeyring) {
    // Not supported yet, see
    // https://github.com/solana-labs/solana/issues/21366.
    return {
      success: false,
      error: getLocale('braveWalletHardwareOperationUnsupportedError'),
      code: undefined
    }
  }
  return {
    success: false,
    error: getLocale('braveWalletUnknownKeyringError'),
    code: undefined
  }
}

export async function signSolTransactionWithHardwareKeyring(
  vendor: BraveWallet.HardwareVendor,
  path: string,
  message: Buffer,
  onAuthorized?: () => void
): Promise<HardwareOperationResultSolanaSignature> {
  const deviceKeyring = getHardwareKeyring(
    vendor,
    BraveWallet.CoinType.SOL,
    onAuthorized
  )

  if (deviceKeyring instanceof SolanaLedgerBridgeKeyring) {
    return deviceKeyring.signTransaction(path, message)
  }

  assertNotReached(`Unsupported  vendor ${vendor}`)
}

export async function cancelHardwareOperation(
  vendor: BraveWallet.HardwareVendor,
  coin: BraveWallet.CoinType
) {
  const deviceKeyring = getHardwareKeyring(vendor, coin)
  if (
    deviceKeyring instanceof EthereumLedgerBridgeKeyring ||
    deviceKeyring instanceof TrezorBridgeKeyring ||
    deviceKeyring instanceof SolanaLedgerBridgeKeyring
  ) {
    return deviceKeyring.cancelOperation()
  }
}

export const loadAccountsFromDevice = async (
  opts: FetchHardwareWalletAccountsProps
): Promise<HardwareOperationResultAccounts> => {
  const keyring = getHardwareKeyring(
    opts.scheme.vendor,
    opts.scheme.coin,
    opts.onAuthorized
  )
  return keyring.getAccounts(opts.startIndex, opts.count, opts.scheme)
}
