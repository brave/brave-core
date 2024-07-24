/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { BraveWallet } from '../../../constants/types'
import { LedgerSolanaKeyring } from '../interfaces'
import {
  AccountFromDevice,
  HardwareImportScheme,
  DerivationScheme,
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult
} from '../types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  LedgerError,
  BtcGetAccountResponse,
  BtcGetAccountResponsePayload
} from './ledger-messages'

import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class BitcoinLedgerBridgeKeyring
  extends LedgerBridgeKeyring
  implements LedgerSolanaKeyring
{
  constructor(onAuthorized?: () => void) {
    super(onAuthorized)
  }

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.BTC
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
    const paths: string[] = []
    for (let i = 0; i < count; i++) {
      paths.push(scheme.pathTemplate(from + i))
    }
    return this.getAccountsFromDevice(paths, scheme)
  }

  signTransaction = async (
    path: string,
    rawTxBytes: Buffer
  ): Promise<SignHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }

    // TODO(apaymyshev): implement
    return { success: true, payload: undefined }
  }

  private readonly getAccountsFromDevice = async (
    paths: string[],
    scheme: HardwareImportScheme
  ): Promise<GetAccountsHardwareOperationResult> => {
    let accounts: AccountFromDevice[] = []
    for (const path of paths) {
      const data = await this.sendCommand<BtcGetAccountResponse>({
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        path: path,
        origin: window.origin,
        xpubVersion:
          scheme.derivationScheme === DerivationScheme.BtcLedgerMainnet
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
        const ledgerError = data.payload as LedgerError
        return {
          success: false,
          error: ledgerError,
          code: ledgerError.statusCode
        }
      }
      const responsePayload = data.payload as BtcGetAccountResponsePayload

      accounts.push({
        address: responsePayload.xpub,
        derivationPath: path
      })
    }
    return { success: true, payload: accounts }
  }
}
