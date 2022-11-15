/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
import { BraveWallet } from '../../../constants/types'
import { getCoinName } from '../../api/hardware_keyrings'
import { LedgerFilecoinKeyring } from '../interfaces'
import {
  FilecoinNetwork, GetAccountsHardwareOperationResult, SignHardwareOperationResult
} from '../types'
import { FilGetAccountResponse, FilGetAccountResponsePayload, FilSignTransactionResponse, FilSignTransactionResponsePayload } from './fil-ledger-messages'
import { LedgerBridgeErrorCodes, LedgerCommand, LedgerError } from './ledger-messages'
import LedgerBridgeKeyring from './ledger_bridge_keyring'

export default class FilecoinLedgerBridgeKeyring extends LedgerBridgeKeyring implements LedgerFilecoinKeyring {
  constructor (onAuthorized?: () => void) {
    super(onAuthorized)
  }

  getAccounts = async (from: number, to: number, network: FilecoinNetwork): Promise<GetAccountsHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }

    from = (from < 0) ? 0 : from
    let accounts = []

    const data = await this.sendCommand<FilGetAccountResponse>({
      command: LedgerCommand.GetAccount,
      id: LedgerCommand.GetAccount,
      from: from,
      to: to,
      network: network,
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
    const responsePayload = data.payload as FilGetAccountResponsePayload

    for (let i = 0; i < responsePayload.accounts.length; i++) {
      accounts.push({
        address: responsePayload.accounts[i],
        derivationPath: this.getPathForIndex(from + i, network),
        name: getCoinName(this.coin()) + ' ' + this.type(),
        hardwareVendor: this.type(),
        deviceId: responsePayload.deviceId,
        coin: this.coin(),
        network: network
      })
    }

    return { success: true, payload: accounts }
  }

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.FIL
  }

  signTransaction = async (message: string): Promise<SignHardwareOperationResult> => {
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

    if (data === LedgerBridgeErrorCodes.BridgeNotReady ||
      data === LedgerBridgeErrorCodes.CommandInProgress) {
      return this.createErrorFromCode(data)
    }

    if (!data.payload.success) {
      const ledgerError = data.payload as LedgerError
      return { success: false, error: ledgerError, code: ledgerError.statusCode }
    }

    try {
      return { success: true, payload: (data.payload as FilSignTransactionResponsePayload).lotusMessage }
    } catch (e) {
      return { success: false, error: e.message, code: e.statusCode || e.id || e.name }
    }
  }

  private readonly getPathForIndex = (index: number, type: FilecoinNetwork): string => {
    // According to SLIP-0044 For TEST networks coin type use 1 always.
    // https://github.com/satoshilabs/slips/blob/5f85bc4854adc84ca2dc5a3ab7f4b9e74cb9c8ab/slip-0044.md
    // https://github.com/glifio/modules/blob/primary/packages/filecoin-wallet-provider/src/utils/createPath/index.ts
    return type === BraveWallet.FILECOIN_MAINNET ? `m/44'/461'/0'/0/${index}` : `m/44'/1'/0'/0/${index}`
  }
}
