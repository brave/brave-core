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
import { getHardwareKeyring, getLedgerHardwareKeyring, getTrezorHardwareKeyring, HardwareVendor } from '../api/hardware_keyrings'
import { TrezorErrorsCodes } from '../hardware/trezor/trezor-messages'
import TrezorBridgeKeyring from '../hardware/trezor/trezor_bridge_keyring'
import LedgerBridgeKeyring from '../hardware/ledgerjs/eth_ledger_bridge_keyring'
import { BraveWallet } from '../../constants/types'

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
  const chainId = await apiProxy.jsonRpcService.getChainId()
  const nonce = await apiProxy.ethTxService.getNonceForHardwareTransaction(txInfo.id)
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
  const { v, r, s } = signed.payload
  const result =
    await apiProxy.ethTxService.processHardwareSignature(txInfo.id, v, r, s)
  if (!result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signLedgerTransaction (
  apiProxy: WalletApiProxy,
  path: string,
  txInfo: BraveWallet.TransactionInfo,
  deviceKeyring: LedgerBridgeKeyring = getLedgerHardwareKeyring(BraveWallet.CoinType.ETH) as LedgerBridgeKeyring): Promise<SignHardwareTransactionOperationResult> {
  const nonce = await apiProxy.ethTxService.getNonceForHardwareTransaction(txInfo.id)
  if (!nonce || !nonce.nonce) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  const data = await apiProxy.ethTxService.getTransactionMessageToSign(txInfo.id)
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
  const { v, r, s } = signed.payload
  const result = await apiProxy.ethTxService.processHardwareSignature(txInfo.id, '0x' + v, '0x' + r, '0x' + s)
  if (!result || !result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signMessageWithHardwareKeyring (vendor: HardwareVendor, path: string, message: string): Promise<SignHardwareMessageOperationResult> {
  const deviceKeyring = getHardwareKeyring(vendor)
  if (deviceKeyring instanceof LedgerBridgeKeyring) {
    return deviceKeyring.signPersonalMessage(path, message)
  } else if (deviceKeyring instanceof TrezorBridgeKeyring) {
    return deviceKeyring.signPersonalMessage(path, message)
  }
  return { success: false, error: getLocale('braveWalletUnknownKeyringError') }
}

export async function cancelHardwareOperation (vendor: HardwareVendor) {
  const deviceKeyring = getHardwareKeyring(vendor)
  if (deviceKeyring instanceof LedgerBridgeKeyring || deviceKeyring instanceof TrezorBridgeKeyring) {
    return deviceKeyring.cancelOperation()
  }
}
