/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
import { LedgerFilecoinKeyring } from '../interfaces'
import {
  AccountFromDevice,
  HardwareImportScheme,
  DerivationSchemes,
  HardwareOperationResultAccounts,
  HardwareOperationResultFilecoinSignature
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import {
  FilGetAccountResponse,
  FilSignTransactionResponse,
  LedgerBridgeErrorCodes,
  LedgerCommand
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
  ): Promise<HardwareOperationResultAccounts> => {
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
      return { ...data.payload }
    }

    let accounts: AccountFromDevice[] = []
    for (let i = 0; i < data.payload.accounts.length; i++) {
      accounts.push({
        address: data.payload.accounts[i],
        derivationPath: scheme.pathTemplate(from + i)
      })
    }

    return { success: true, accounts: accounts }
  }

  signTransaction = async (
    message: string
  ): Promise<HardwareOperationResultFilecoinSignature> => {
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
      return { ...data.payload }
    }

    try {
      return {
        success: true,
        signature: {
          // TODO(apaymyshev): should have trusted->untrusted checks?
          signedMessageJson: data.payload.untrustedSignedTxJson
        }
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
