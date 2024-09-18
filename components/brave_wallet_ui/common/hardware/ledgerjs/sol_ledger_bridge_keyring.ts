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
  HardwareOperationResultAccounts,
  HardwareOperationResultSolanaSignature
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  SolGetAccountResponse,
  SolSignTransactionResponse
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
  ): Promise<HardwareOperationResultAccounts> => {
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
  ): Promise<HardwareOperationResultSolanaSignature> => {
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
      return { ...data.payload }
    }
    return {
      success: true,
      // TODO(apaymyshev): should have trusted->untrusted checks?
      signature: { bytes: [...data.payload.untrustedSignatureBytes] }
    }
  }

  private readonly getAccountsFromDevice = async (
    paths: string[]
  ): Promise<HardwareOperationResultAccounts> => {
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
        return { ...data.payload }
      }
      accounts.push({
        address: bs58.encode(data.payload.address),
        derivationPath: path
      })
    }
    return {
      success: true,
      accounts: accounts
    }
  }
}
