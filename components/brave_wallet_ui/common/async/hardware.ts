// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  TransportStatusError as LedgerTransportStatusError,
  TransportError as LedgerTransportError,
  DisconnectedDeviceDuringOperation as LedgerDisconnectedDeviceDuringOperation,
  StatusCodes as LedgerStatusCodes
} from '@ledgerhq/errors'

import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { SignHardwareTransactionType, SignHardwareMessageOperationResult } from '../hardware_operations'
import { getLocale } from '../../../common/locale'
import WalletApiProxy from '../../common/wallet_api_proxy'
import LedgerBridgeKeyring from '../../common/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../../common/trezor/trezor_bridge_keyring'
import { HardwareWalletErrorType } from '../../constants/types'
import { TrezorErrorsCodes } from '../trezor/trezor-messages'

export async function signTrezorTransaction (apiProxy: WalletApiProxy, path: string, txInfo: BraveWallet.TransactionInfo): Promise<SignHardwareTransactionType> {
  const chainId = await apiProxy.ethJsonRpcController.getChainId()
  const nonce = await apiProxy.ethTxController.getNonceForHardwareTransaction(txInfo.id)
  if (!nonce || !nonce.nonce) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  txInfo.txData.baseData.nonce = nonce.nonce
  const deviceKeyring = apiProxy.getKeyringsByType(BraveWallet.TREZOR_HARDWARE_VENDOR) as TrezorBridgeKeyring
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
    await apiProxy.ethTxController.processHardwareSignature(txInfo.id, v, r, s)
  if (!result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signLedgerTransaction (apiProxy: WalletApiProxy, path: string, txInfo: BraveWallet.TransactionInfo): Promise<SignHardwareTransactionType> {
  const nonce = await apiProxy.ethTxController.getNonceForHardwareTransaction(txInfo.id)
  if (!nonce || !nonce.nonce) {
    return { success: false, error: getLocale('braveWalletApproveTransactionError') }
  }
  const data = await apiProxy.ethTxController.getTransactionMessageToSign(txInfo.id)
  if (!data || !data.message) {
    return { success: false, error: getLocale('braveWalletNoMessageToSignError') }
  }
  const deviceKeyring = apiProxy.getKeyringsByType(BraveWallet.LEDGER_HARDWARE_VENDOR) as LedgerBridgeKeyring

  let signed
  try {
    signed = await deviceKeyring.signTransaction(path, data.message.replace('0x', ''))
  } catch (e) {
    const ledgerError = parseLedgerDeviceError(e)
    if (ledgerError === 'needsConnectionReset') {
      await deviceKeyring.makeApp()
    }

    return { success: false, deviceError: ledgerError }
  }

  if (!signed || !signed.success || !signed.payload) {
    const error = signed && signed.error ? signed.error : getLocale('braveWalletSignOnDeviceError')
    return { success: false, error: error }
  }
  const { v, r, s } = signed.payload
  const result = await apiProxy.ethTxController.processHardwareSignature(txInfo.id, '0x' + v, '0x' + r, '0x' + s)
  if (!result || !result.status) {
    return { success: false, error: getLocale('braveWalletProcessTransactionError') }
  }
  return { success: result.status }
}

export async function signMessageWithHardwareKeyring (apiProxy: WalletApiProxy, vendor: string, path: string, message: string): Promise<SignHardwareMessageOperationResult> {
  const deviceKeyring = await apiProxy.getKeyringsByType(vendor)
  if (deviceKeyring instanceof LedgerBridgeKeyring) {
    return deviceKeyring.signPersonalMessage(path, message)
  } else if (deviceKeyring instanceof TrezorBridgeKeyring) {
    return deviceKeyring.signPersonalMessage(path, message)
  }
  return { success: false, error: getLocale('braveWalletUnknownKeyringError') }
}

export function parseLedgerDeviceError (e: any): HardwareWalletErrorType {
  if (e instanceof LedgerTransportStatusError && (e as any).statusCode === LedgerStatusCodes.CONDITIONS_OF_USE_NOT_SATISFIED) {
    return 'transactionRejected'
  }

  if (e instanceof LedgerTransportError && e.message === 'Ledger Device is busy (lock signTransaction)') {
    return 'deviceBusy'
  }

  if (e instanceof LedgerDisconnectedDeviceDuringOperation) {
    return 'needsConnectionReset'
  }

  return 'deviceNotConnected'
}
