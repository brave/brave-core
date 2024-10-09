/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LedgerBitcoinKeyring } from '../interfaces'
import {
  AccountFromDevice,
  HardwareImportScheme,
  DerivationSchemes,
  HardwareOperationResultAccounts,
  HardwareOperationResultBitcoinSignature
} from '../types'
import { BridgeType, BridgeTypes } from '../untrusted_shared_types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  BtcGetAccountResponse,
  BtcSignTransactionResponse
} from './ledger-messages'

import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class BitcoinLedgerBridgeKeyring
  extends LedgerBridgeKeyring
  implements LedgerBitcoinKeyring
{
  constructor(onAuthorized?: () => void) {
    super(onAuthorized)
  }

  bridgeType = (): BridgeType => {
    return BridgeTypes.BtcLedger
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
    const paths: string[] = []
    for (let i = 0; i < count; i++) {
      paths.push(scheme.pathTemplate(from + i))
    }
    return this.getAccountsFromDevice(paths, scheme)
  }

  private readonly getAccountsFromDevice = async (
    paths: string[],
    scheme: HardwareImportScheme
  ): Promise<HardwareOperationResultAccounts> => {
    let accounts: AccountFromDevice[] = []
    for (const path of paths) {
      const data = await this.sendCommand<BtcGetAccountResponse>({
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        path: path,
        origin: window.origin,
        xpubVersion:
          scheme.derivationScheme === DerivationSchemes.BtcLedgerMainnet
            ? 0x0488b21e // xpub
            : 0x043587cf // tpub
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
        address: data.payload.xpub,
        derivationPath: path
      })
    }
    return { success: true, accounts: accounts }
  }

  signTransaction = async (
    inputTransactions: Array<{
      txBytes: Buffer
      outputIndex: number
      associatedPath: string
    }>,
    outputScript: Buffer,
    changePath: string | undefined,
    lockTime: number
  ): Promise<HardwareOperationResultBitcoinSignature> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    const data = await this.sendCommand<BtcSignTransactionResponse>({
      command: LedgerCommand.SignTransaction,
      id: LedgerCommand.SignTransaction,
      origin: window.origin,
      inputTransactions,
      outputScript,
      changePath,
      lockTime
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
      signature: { witnessArray: data.payload.witnesses.map((w) => [...w]) }
    }
  }
}
