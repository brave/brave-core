/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from 'chrome://resources/js/assert_ts.js'
import { BraveWallet } from '../../../constants/types'
import { LedgerSolanaKeyring } from '../interfaces'
import {
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult,
  SolDerivationPaths
} from '../types'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  LedgerError
} from './ledger-messages'
import {
  SolGetAccountResponse,
  SolGetAccountResponsePayload,
  SolSignTransactionResponse,
  SolSignTransactionResponsePayload
} from './sol-ledger-messages'

import { hardwareDeviceIdFromAddress } from '../hardwareDeviceIdFromAddress'
import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class SolanaLedgerBridgeKeyring extends LedgerBridgeKeyring implements LedgerSolanaKeyring {
  constructor (onAuthorized?: () => void) {
    super(onAuthorized)
  }

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.SOL
  }

  getAccounts = async (from: number, to: number, scheme: SolDerivationPaths): Promise<GetAccountsHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }
    from = (from >= 0) ? from : 0
    const paths = []
    const addZeroPath = (from > 0 || to < 0)
    if (addZeroPath) {
      // Add zero address to calculate device id.
      paths.push(this.getPathForIndex(0, scheme))
    }
    for (let i = from; i <= to; i++) {
      paths.push(this.getPathForIndex(i, scheme))
    }
    return this.getAccountsFromDevice(paths, addZeroPath, scheme)
  }

  signTransaction = async (path: string, rawTxBytes: Buffer): Promise<SignHardwareOperationResult> => {
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
    if (data === LedgerBridgeErrorCodes.BridgeNotReady ||
        data === LedgerBridgeErrorCodes.CommandInProgress) {
      return this.createErrorFromCode(data)
    }
    if (!data.payload.success) {
      // TODO Either pass data.payload (LedgerError) or data.payload.message (LedgerError.message)
      // consistently here and in getAccountsFromDevice.  Currently we pass the entire LedgerError up
      // to UI only for getAccounts to make statusCode available, but don't do the same here
      // for signTransaction.
      const ledgerError = data.payload as LedgerError
      return { success: false, error: ledgerError.message, code: ledgerError.statusCode }
    }
    const responsePayload = data.payload as SolSignTransactionResponsePayload
    return { success: true, payload: responsePayload.signature }
  }

  private readonly getAccountsFromDevice = async (paths: string[], skipZeroPath: boolean, scheme: SolDerivationPaths): Promise<GetAccountsHardwareOperationResult> => {
    let accounts = []
    const zeroPath = this.getPathForIndex(0, scheme)
    for (const path of paths) {
      const data = await this.sendCommand<SolGetAccountResponse>({
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        path: path,
        origin: window.origin
      })
      if (data === LedgerBridgeErrorCodes.BridgeNotReady ||
          data === LedgerBridgeErrorCodes.CommandInProgress) {
        return this.createErrorFromCode(data)
      }

      if (!data.payload.success) {
        const ledgerError = data.payload as LedgerError
        return { success: false, error: ledgerError, code: ledgerError.statusCode }
      }
      const responsePayload = data.payload as SolGetAccountResponsePayload

      if (path === zeroPath) {
        this.deviceId = await hardwareDeviceIdFromAddress(responsePayload.address)
        if (skipZeroPath) {
          // If requested addresses do not have zero indexed adress we add it
          // intentionally to calculate device id and should not add it to
          // returned accounts
          continue
        }
      }

      accounts.push({
        address: '',
        addressBytes: responsePayload.address,
        derivationPath: path,
        name: this.type(),
        hardwareVendor: this.type(),
        deviceId: this.deviceId,
        coin: this.coin(),
        network: undefined
      })
    }
    return { success: true, payload: accounts }
  }

  private readonly getPathForIndex = (index: number, scheme: SolDerivationPaths): string => {
    if (scheme === SolDerivationPaths.LedgerLive) {
      return `44'/501'/${index}'`
    }
    assert(scheme === SolDerivationPaths.Default, '')

    return `44'/501'/${index}'/0'`
  }
}
