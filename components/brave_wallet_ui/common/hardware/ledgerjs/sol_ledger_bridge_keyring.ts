/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as bs58 from 'bs58'
import { LedgerSolanaKeyring } from '../interfaces'
import {
  AccountFromDevice,
  HardwareImportScheme,
  DerivationSchemes,
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  LedgerError,
  SolGetAccountResponse,
  SolGetAccountResponsePayload,
  SolSignTransactionResponse,
  SolSignTransactionResponsePayload
} from './ledger-messages'

import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class SolanaLedgerBridgeKeyring
  extends LedgerBridgeKeyring
  implements LedgerSolanaKeyring
{
  constructor(onAuthorized?: () => void) {
    super(onAuthorized)
  }

  bridgeType = (): BridgeType => {
    return BridgeTypes.SolLedger
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
    // The root path does not support an index
    if (scheme.derivationScheme === DerivationSchemes.SolLedgerBip44Root) {
      return this.getAccountsFromDevice([scheme.pathTemplate(0)])
    }

    const paths: string[] = []
    for (let i = 0; i < count; i++) {
      paths.push(scheme.pathTemplate(from + i))
    }
    return this.getAccountsFromDevice(paths)
  }

  signTransaction = async (
    path: string,
    rawTxBytes: Buffer
  ): Promise<SignHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }

    const data = await this.sendCommand<SolSignTransactionResponse>({
      command: LedgerCommand.SignTransaction,
      id: LedgerCommand.SignTransaction,
      path: path,
      rawTxBytes: rawTxBytes,
      origin: window.origin
    })
    if (
      data === LedgerBridgeErrorCodes.BridgeNotReady ||
      data === LedgerBridgeErrorCodes.CommandInProgress
    ) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      // TODO Either pass data.payload (LedgerError) or data.payload.message
      // (LedgerError.message) consistently here and in getAccountsFromDevice.
      // Currently we pass the entire LedgerError up to UI only for getAccounts
      // to make statusCode available, but don't do the same here for
      // signTransaction.
      const ledgerError = data.payload as LedgerError
      return {
        success: false,
        error: ledgerError.message,
        code: ledgerError.statusCode
      }
    }
    const responsePayload = data.payload as SolSignTransactionResponsePayload
    return { success: true, payload: responsePayload.signature }
  }

  private readonly getAccountsFromDevice = async (
    paths: string[]
  ): Promise<GetAccountsHardwareOperationResult> => {
    let accounts: AccountFromDevice[] = []

    for (const path of paths) {
      const data = await this.sendCommand<SolGetAccountResponse>({
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
      const responsePayload = data.payload as SolGetAccountResponsePayload

      accounts.push({
        address: bs58.encode(responsePayload.address),
        derivationPath: path
      })
    }
    return { success: true, payload: accounts }
  }
}
