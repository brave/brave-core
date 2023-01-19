// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  SignHardwareTransactionType,
  SignHardwareOperationResult,
  HardwareWalletResponseCodeType
} from '../hardware/types'
import { StatusCodes as LedgerStatusCodes } from '@ledgerhq/errors'
import { getLocale } from '../../../common/locale'
import WalletApiProxy from '../../common/wallet_api_proxy'
import {
  getHardwareKeyring,
  getLedgerEthereumHardwareKeyring,
  getLedgerFilecoinHardwareKeyring,
  getLedgerSolanaHardwareKeyring,
  getTrezorHardwareKeyring,
  HardwareVendor
} from '../api/hardware_keyrings'
import { TrezorErrorsCodes } from '../hardware/trezor/trezor-messages'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import EthereumLedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import SolanaLedgerBridgeKeyring from '../hardware/ledgerjs/sol_ledger_bridge_keyring'
import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'
import { LedgerEthereumKeyring, LedgerFilecoinKeyring, LedgerSolanaKeyring } from '../hardware/interfaces'
import { EthereumSignedTx } from '../hardware/trezor/trezor-connect-types'
import FilecoinLedgerBridgeKeyring from '../hardware/ledgerjs/fil_ledger_bridge_keyring'
import { FilSignedLotusMessage } from '../hardware/ledgerjs/fil-ledger-messages'

export function dialogErrorFromLedgerErrorCode (code: string | number): HardwareWalletResponseCodeType {
  if (code === 'TransportOpenUserCancelled') {
    return 'deviceNotConnected'
  }

  if (code === 'TransportLocked') {
    return 'deviceBusy'
  }

  if (code === LedgerStatusCodes.CONDITIONS_OF_USE_NOT_SATISFIED) {
    return 'transactionRejected'
  }

  return 'openLedgerApp'
}

export function dialogErrorFromTrezorErrorCode (code: TrezorErrorsCodes | string): HardwareWalletResponseCodeType {
  if (code === TrezorErrorsCodes.CommandInProgress) {
    return 'deviceBusy'
  }
  if (code === 'Method_Interrupted') {
    return 'transactionRejected'
  }
  return 'openLedgerApp'
}

export async function signTrezorTransaction (
  apiProxy: WalletApiProxy,
  path: string,
  txInfo: SerializableTransactionInfo,
  deviceKeyring: TrezorBridgeKeyring = getTrezorHardwareKeyring()): Promise<SignHardwareTransactionType> {
  const chainId = await apiProxy.jsonRpcService.getChainId(BraveWallet.CoinType.ETH)
  const nonce = await apiProxy.ethTxManagerProxy.getNonceForHardwareTransaction(txInfo.id)
  if (!nonce || !nonce.nonce) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  if (!txInfo.txDataUnion.ethTxData1559) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  const tx = {
    ...txInfo,
    txDataUnion: {
      ...txInfo.txDataUnion,
      ethTxData1559: {
        ...txInfo.txDataUnion.ethTxData1559,
        baseData: {
          ...txInfo.txDataUnion.ethTxData1559?.baseData,
          nonce: nonce.nonce
        }
      }
    }
  } as SerializableTransactionInfo
  const signed = await deviceKeyring.signTransaction(path, tx, chainId.chainId)
  if (!signed || !signed.success || !signed.payload) {
    const error = (signed.error ? signed.error : getLocale('braveWalletSignOnDeviceError')) as string
    if (signed.code === TrezorErrorsCodes.CommandInProgress) {
      return { success: false, error: error, deviceError: 'deviceBusy' }
    }
    return { success: false, error: error }
  }
  const ethereumSignedTx = signed.payload as EthereumSignedTx
  if (!ethereumSignedTx) {
    return { success: false }
  }
  const { v, r, s } = ethereumSignedTx
  const result =
    await apiProxy.ethTxManagerProxy.processHardwareSignature(tx.id, v, r, s)
  if (!result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signLedgerEthereumTransaction (
  apiProxy: WalletApiProxy,
  path: string,
  txInfo: SerializableTransactionInfo,
  coin: BraveWallet.CoinType,
  deviceKeyring: LedgerEthereumKeyring = getLedgerEthereumHardwareKeyring()): Promise<SignHardwareOperationResult> {
  const nonce = await apiProxy.ethTxManagerProxy.getNonceForHardwareTransaction(txInfo.id)
  if (!nonce || !nonce.nonce) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  const data = await apiProxy.txService.getTransactionMessageToSign(coin, txInfo.id)
  if (!data || !data.message || !data.message.messageStr) {
    return { success: false, error: getLocale('braveWalletNoMessageToSignError') }
  }

  const signed = await deviceKeyring.signTransaction(path, data.message.messageStr?.replace('0x', ''))

  if (!signed || !signed.success || !signed.payload) {
    const error = signed?.error ?? getLocale('braveWalletSignOnDeviceError')
    const code = signed?.code ?? ''
    return { success: false, error: error, code: code }
  }
  const { v, r, s } = signed.payload as EthereumSignedTx
  const result = await apiProxy.ethTxManagerProxy.processHardwareSignature(txInfo.id, '0x' + v, '0x' + r, '0x' + s)
  if (!result || !result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signLedgerFilecoinTransaction (
  apiProxy: WalletApiProxy,
  txInfo: SerializableTransactionInfo,
  coin: BraveWallet.CoinType,
  deviceKeyring: LedgerFilecoinKeyring = getLedgerFilecoinHardwareKeyring()): Promise<SignHardwareOperationResult> {
  const data = await apiProxy.txService.getTransactionMessageToSign(coin, txInfo.id)
  if (!data || !data.message || !data.message.messageStr) {
    return { success: false, error: getLocale('braveWalletNoMessageToSignError') }
  }

  const signed = await deviceKeyring.signTransaction(data.message.messageStr)
  if (!signed || !signed.success || !signed.payload) {
    const error = signed?.error ?? getLocale('braveWalletSignOnDeviceError')
    const code = signed?.code ?? ''
    return { success: false, error: error, code: code }
  }
  const signedMessage = signed.payload as FilSignedLotusMessage
  if (!signedMessage) {
    return { success: false }
  }

  const result = await apiProxy.filTxManagerProxy.processFilHardwareSignature(txInfo.id, JSON.stringify(signedMessage))
  if (!result || !result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signLedgerSolanaTransaction (
  apiProxy: WalletApiProxy,
  path: string,
  txId: string,
  coin: BraveWallet.CoinType,
  deviceKeyring: LedgerSolanaKeyring = getLedgerSolanaHardwareKeyring()): Promise<SignHardwareOperationResult> {
    const data = await apiProxy.txService.getTransactionMessageToSign(coin, txId)
    if (!data || !data.message || !data.message.messageBytes) {
      return { success: false, error: getLocale('braveWalletNoMessageToSignError') }
    }
    const signed = await deviceKeyring.signTransaction(path, Buffer.from(data.message.messageBytes))
    if (!signed || !signed.success || !signed.payload) {
      const error = signed?.error ?? getLocale('braveWalletSignOnDeviceError')
      const code = signed?.code ?? ''
      return { success: false, error: error, code: code }
    }

    const signedMessage = signed.payload as Buffer
    if (!signedMessage) {
      return { success: false }
    }

    const result = await apiProxy.solanaTxManagerProxy.processSolanaHardwareSignature(txId, [...signedMessage])
    if (!result || !result.status) {
      return { success: false, error: getLocale('braveWalletProcessTransactionError') }
    }

    return { success: result.status }
}

export async function signMessageWithHardwareKeyring (vendor: HardwareVendor, path: string, messageData: Omit<BraveWallet.SignMessageRequest, 'originInfo'>): Promise<SignHardwareOperationResult> {
  const deviceKeyring = getHardwareKeyring(vendor, messageData.coin)
  if (deviceKeyring instanceof EthereumLedgerBridgeKeyring) {
    if (messageData.isEip712) {
      if (!messageData.domainHash || !messageData.primaryHash) {
        return { success: false, error: getLocale('braveWalletUnknownInternalError') }
      }
      return deviceKeyring.signEip712Message(path, messageData.domainHash, messageData?.primaryHash)
    }
    return deviceKeyring.signPersonalMessage(path, messageData.message)
  } else if (deviceKeyring instanceof TrezorBridgeKeyring) {
    if (messageData.isEip712) {
      if (!messageData.domainHash || !messageData.primaryHash) {
        return { success: false, error: getLocale('braveWalletUnknownInternalError') }
      }
      return deviceKeyring.signEip712Message(path, messageData.domainHash, messageData.primaryHash)
    }
    return deviceKeyring.signPersonalMessage(path, messageData.message)
  } else if (deviceKeyring instanceof SolanaLedgerBridgeKeyring) {
    // Not supported yet, see https://github.com/solana-labs/solana/issues/21366.
    return { success: false, error: getLocale('braveWalletHardwareOperationUnsupportedError') }
  }
  return { success: false, error: getLocale('braveWalletUnknownKeyringError') }
}

export async function signRawTransactionWithHardwareKeyring (vendor: HardwareVendor, path: string, message: BraveWallet.ByteArrayStringUnion, coin: BraveWallet.CoinType, onAuthorized?: () => void): Promise<SignHardwareOperationResult> {
  const deviceKeyring = getHardwareKeyring(vendor, coin, onAuthorized)

  if (deviceKeyring instanceof SolanaLedgerBridgeKeyring && message.bytes) {
    return deviceKeyring.signTransaction(path, Buffer.from(message.bytes))
  } else if (deviceKeyring instanceof TrezorBridgeKeyring || deviceKeyring instanceof EthereumLedgerBridgeKeyring || deviceKeyring instanceof FilecoinLedgerBridgeKeyring) {
    return { success: false, error: getLocale('braveWalletHardwareOperationUnsupportedError') }
  }

  return { success: false, error: getLocale('braveWalletUnknownKeyringError') }
}

export async function cancelHardwareOperation (vendor: HardwareVendor, coin: BraveWallet.CoinType) {
  const deviceKeyring = getHardwareKeyring(vendor, coin)
  if (deviceKeyring instanceof EthereumLedgerBridgeKeyring || deviceKeyring instanceof TrezorBridgeKeyring || deviceKeyring instanceof SolanaLedgerBridgeKeyring) {
    return deviceKeyring.cancelOperation()
  }
}
