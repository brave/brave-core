/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
import { BraveWallet, FilecoinNetwork } from '../../../constants/types'
import { getPathForFilLedgerIndex } from '../../../utils/derivation_path_utils'
import { getCoinName } from '../../api/hardware_keyrings'
import { LedgerFilecoinKeyring } from '../interfaces'
import {
  GetAccountsHardwareOperationResult,
  SignHardwareOperationResult
} from '../types'
import {
  FilGetAccountResponse,
  FilGetAccountResponsePayload,
  FilSignTransactionResponse,
  FilSignTransactionResponsePayload
} from './fil-ledger-messages'
import {
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

  getAccounts = async (
    from: number,
    to: number,
    network: FilecoinNetwork
  ): Promise<GetAccountsHardwareOperationResult> => {
    const result = await this.unlock()
    if (!result.success) {
      return result
    }

    from = from < 0 ? 0 : from
    let accounts = []

    const data = await this.sendCommand<FilGetAccountResponse>({
      command: LedgerCommand.GetAccount,
      id: LedgerCommand.GetAccount,
      from: from,
      to: to,
      network: network,
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

    for (let i = 0; i < responsePayload.accounts.length; i++) {
      accounts.push({
        address: responsePayload.accounts[i],
        derivationPath: this.getPathForIndex(from + i, network),
        name: getCoinName(this.coin()) + ' ' + this.type(),
        hardwareVendor: this.type(),
        deviceId: responsePayload.deviceId,
        coin: this.coin(),
        keyringId: this.keyringId(network)
      })
    }

    return { success: true, payload: accounts }
  }

  coin = (): BraveWallet.CoinType => {
    return BraveWallet.CoinType.FIL
  }

  keyringId = (network: FilecoinNetwork): BraveWallet.KeyringId => {
    return network === BraveWallet.FILECOIN_MAINNET
      ? BraveWallet.KeyringId.kFilecoin
      : BraveWallet.KeyringId.kFilecoinTestnet
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

  private readonly getPathForIndex = getPathForFilLedgerIndex
}
