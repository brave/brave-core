/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
import { LedgerFilecoinKeyring } from '../interfaces'
import {
  AccountFromDevice,
  HardwareImportScheme,
  DerivationSchemes,
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import {
  FilGetAccountResponse,
  FilGetAccountResponsePayload,
  FilSignTransactionResponse,
  FilSignTransactionResponsePayload,
  LedgerBridgeErrorCodes,
  LedgerCommand,
  LedgerError
} from './ledger-messages'
import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class FilecoinLedgerBridgeKeyring
  extends LedgerBridgeKeyring
  implements LedgerFilecoinKeyring
{
  constructor(onAuthorized?: () => void) {
    super(onAuthorized)
  }

  bridgeType = (): BridgeType => {
    return BridgeTypes.FilLedger
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

    const isTestnet =
      scheme.derivationScheme === DerivationSchemes.FilLedgerTestnet

    const data = await this.sendCommand<FilGetAccountResponse>({
      command: LedgerCommand.GetAccount,
      id: LedgerCommand.GetAccount,
      from: from,
      count: count,
      isTestnet,
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
    const responsePayload = data.payload as FilGetAccountResponsePayload

    let accounts: AccountFromDevice[] = []
    for (let i = 0; i < responsePayload.accounts.length; i++) {
      accounts.push({
        address: responsePayload.accounts[i],
        derivationPath: scheme.pathTemplate(from + i)
      })
    }

    return { success: true, payload: accounts }
  }

  signTransaction = async (
    message: string
  ): Promise<SignHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }

    const data = await this.sendCommand<FilSignTransactionResponse>({
      command: LedgerCommand.SignTransaction,
      id: LedgerCommand.SignTransaction,
      message: message,
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

    try {
      return {
        success: true,
        payload: (data.payload as FilSignTransactionResponsePayload)
          .lotusMessage
      }
    } catch (e) {
      return {
        success: false,
        error: e.message,
        code: e.statusCode || e.id || e.name
      }
    }
  }
}
