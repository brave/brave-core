// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  SignHardwareTransactionType,
  SignHardwareMessageOperationResult,
  HardwareWalletResponseCodeType,
  SignHardwareTransactionOperationResult
} from '../hardware/types'
import { StatusCodes as LedgerStatusCodes } from '@ledgerhq/errors'
import { getLocale } from '../../../common/locale'
import WalletApiProxy from '../../common/wallet_api_proxy'
import { getHardwareKeyring, getLedgerEthereumHardwareKeyring, getLedgerFilecoinHardwareKeyring, getTrezorHardwareKeyring, HardwareVendor } from '../api/hardware_keyrings'
import { TrezorErrorsCodes } from '../hardware/trezor/trezor-messages'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import LedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import { BraveWallet } from '../../constants/types'
import { LedgerEthereumKeyring, LedgerFilecoinKeyring } from '../hardware/interfaces'
import { EthereumSignedTx } from 'trezor-connect'
import { SignedLotusMessage } from '@glif/filecoin-message'

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

  return 'openEthereumApp'
}

export function dialogErrorFromTrezorErrorCode (code: TrezorErrorsCodes | string): HardwareWalletResponseCodeType {
  if (code === TrezorErrorsCodes.CommandInProgress) {
    return 'deviceBusy'
  }
  if (code === 'Method_Interrupted') {
    return 'transactionRejected'
  }
  return 'openEthereumApp'
}

export async function signTrezorTransaction (
  apiProxy: WalletApiProxy,
  path: string,
  txInfo: BraveWallet.TransactionInfo,
  deviceKeyring: TrezorBridgeKeyring = getTrezorHardwareKeyring()): Promise<SignHardwareTransactionType> {
  const chainId = await apiProxy.jsonRpcService.getChainId(BraveWallet.CoinType.ETH)
  const nonce = await apiProxy.ethTxManagerProxy.getNonceForHardwareTransaction(txInfo.id)
  if (!nonce || !nonce.nonce) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  if (!txInfo.txDataUnion.ethTxData1559) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  txInfo.txDataUnion.ethTxData1559.baseData.nonce = nonce.nonce
  const signed = await deviceKeyring.signTransaction(path, txInfo, chainId.chainId)
  if (!signed || !signed.success || !signed.payload) {
    const error = signed.error ? signed.error : getLocale('braveWalletSignOnDeviceError')
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
    await apiProxy.ethTxManagerProxy.processHardwareSignature(txInfo.id, v, r, s)
  if (!result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signLedgerEthereumTransaction (
  apiProxy: WalletApiProxy,
  path: string,
  txInfo: BraveWallet.TransactionInfo,
  coin: BraveWallet.CoinType,
  deviceKeyring: LedgerEthereumKeyring = getLedgerEthereumHardwareKeyring()): Promise<SignHardwareTransactionOperationResult> {
  const nonce = await apiProxy.ethTxManagerProxy.getNonceForHardwareTransaction(txInfo.id)
  if (!nonce || !nonce.nonce) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  const data = await apiProxy.txService.getTransactionMessageToSign(coin, txInfo.id)
  if (!data || !data.message) {
    return { success: false, error: getLocale('braveWalletNoMessageToSignError') }
  }

  const signed = await deviceKeyring.signTransaction(path, data.message.replace('0x', ''))
  if (!signed || !signed.success || !signed.payload) {
    const error = signed?.error ?? getLocale('braveWalletSignOnDeviceError')
    const code = signed?.code ?? ''
    if (code === 'DisconnectedDeviceDuringOperation') {
      await deviceKeyring.makeApp()
    }
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
  txInfo: BraveWallet.TransactionInfo,
  coin: BraveWallet.CoinType,
  deviceKeyring: LedgerFilecoinKeyring = getLedgerFilecoinHardwareKeyring()): Promise<SignHardwareTransactionOperationResult> {
  const data = await apiProxy.txService.getTransactionMessageToSign(coin, txInfo.id)
  if (!data || !data.message) {
    return { success: false, error: getLocale('braveWalletNoMessageToSignError') }
  }

  const signed = await deviceKeyring.signTransaction(data.message)
  if (!signed || !signed.success || !signed.payload) {
    const error = signed?.error ?? getLocale('braveWalletSignOnDeviceError')
    const code = signed?.code ?? ''
    if (code === 'DisconnectedDeviceDuringOperation') {
      await deviceKeyring.makeApp()
    }
    return { success: false, error: error, code: code }
  }
  const signedMessage = signed.payload as SignedLotusMessage
  if (!signedMessage) {
    return { success: false }
  }

  const result = await apiProxy.filTxManagerProxy.processFilHardwareSignature(txInfo.id, JSON.stringify(signedMessage))
  if (!result || !result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signMessageWithHardwareKeyring (vendor: HardwareVendor, path: string, messageData: BraveWallet.SignMessageRequest): Promise<SignHardwareMessageOperationResult> {
  const deviceKeyring = getHardwareKeyring(vendor)
  if (deviceKeyring instanceof LedgerBridgeKeyring) {
    if (messageData.isEip712) {
      if (!messageData.domainHash || !messageData.primaryHash) {
        return { success: false, error: getLocale('braveWalletUnknownInternalError') }
      }
      return deviceKeyring.signEip712Message(path, messageData.domainHash, messageData?.primaryHash)
    } else return deviceKeyring.signPersonalMessage(path, messageData.message)
  } else if (deviceKeyring instanceof TrezorBridgeKeyring) {
    if (messageData.isEip712) {
      if (!messageData.domainHash || !messageData.primaryHash) {
        return { success: false, error: getLocale('braveWalletUnknownInternalError') }
      }
      return deviceKeyring.signEip712Message(path, messageData.domainHash, messageData.primaryHash)
    } else return deviceKeyring.signPersonalMessage(path, messageData.message)
  }
  return { success: false, error: getLocale('braveWalletUnknownKeyringError') }
}

export async function cancelHardwareOperation (vendor: HardwareVendor) {
  const deviceKeyring = getHardwareKeyring(vendor)
  if (deviceKeyring instanceof LedgerBridgeKeyring || deviceKeyring instanceof TrezorBridgeKeyring) {
    return deviceKeyring.cancelOperation()
  }
}
