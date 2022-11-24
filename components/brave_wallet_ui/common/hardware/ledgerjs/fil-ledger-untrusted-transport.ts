/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CoinType } from '@glif/filecoin-address'
import { LotusMessage, SignedLotusMessage } from '@glif/filecoin-message'
import { LedgerProvider, TransportWrapper } from '@glif/filecoin-wallet-provider'
import { BraveWallet } from '../../../constants/types'
import {
  FilGetAccountCommand,
  FilGetAccountResponse, FilSignTransactionCommand, FilSignTransactionResponse
} from './fil-ledger-messages'
import {
  LedgerBridgeErrorCodes,
  LedgerCommand, UnlockResponse
} from './ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

// FilecoinLedgerUntrustedMessagingTransport makes calls to the Filecoin app on a Ledger device
export class FilecoinLedgerUntrustedMessagingTransport extends LedgerUntrustedMessagingTransport {
  transportWrapper?: TransportWrapper
  provider?: LedgerProvider
  deviceId: string

  constructor (targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(LedgerCommand.Unlock, this.handleUnlock)
    this.addCommandHandler<FilGetAccountResponse>(LedgerCommand.GetAccount, this.handleGetAccount)
    this.addCommandHandler<FilSignTransactionResponse>(LedgerCommand.SignTransaction, this.handleSignTransaction)
  }

  private handleGetAccount = async (command: FilGetAccountCommand): Promise<FilGetAccountResponse> => {
    try {
      if (!this.provider && !await this.makeProvider()) {
        return {
          command: LedgerCommand.GetAccount,
          id: LedgerCommand.GetAccount,
          origin: command.origin,
          payload: {
            success: false,
            statusCode: LedgerBridgeErrorCodes.BridgeNotReady
          }
        }
      }

      const coinType = command.network === BraveWallet.FILECOIN_TESTNET ? CoinType.TEST : CoinType.MAIN
      const accounts = await this.provider!.getAccounts(command.from, command.to, coinType)

      return {
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        origin: command.origin,
        payload: {
          success: true,
          accounts: accounts,
          deviceId: this.deviceId
        }
      }
    } catch (e) {
      return {
        command: LedgerCommand.GetAccount,
        id: LedgerCommand.GetAccount,
        origin: command.origin,
        payload: e
      }
    }
  }

  private handleSignTransaction = async (command: FilSignTransactionCommand): Promise<FilSignTransactionResponse> => {
    try {
      if (!this.provider && !await this.makeProvider()) {
        return {
          command: LedgerCommand.SignTransaction,
          id: LedgerCommand.SignTransaction,
          origin: command.origin,
          payload: {
            success: false,
            statusCode: LedgerBridgeErrorCodes.BridgeNotReady
          }
        }
      }
      // Accounts should be warmed up before sign in
      await this.provider!.getAccounts()
      const parsed = JSON.parse(command.message)
      const lotusMessage: LotusMessage = {
        To: parsed.To,
        From: parsed.From,
        Nonce: parsed.Nonce,
        Value: parsed.Value,
        GasPremium: parsed.GasPremium,
        GasLimit: parsed.GasLimit,
        GasFeeCap: parsed.GasFeeCap,
        Method: parsed.Method,
        Params: parsed.Params
      }
      const signed: SignedLotusMessage = await this.provider!.sign(parsed.From, lotusMessage)
      return {
        command: LedgerCommand.SignTransaction,
        id: LedgerCommand.SignTransaction,
        origin: command.origin,
        payload: {
          success: true,
          lotusMessage: signed
        }
      }
    } catch (e) {
      return {
        command: LedgerCommand.SignTransaction,
        id: LedgerCommand.SignTransaction,
        origin: command.origin,
        payload: e
      }
    }
  }

  private makeProvider = async (): Promise<Boolean> => {
    if (this.transportWrapper) {
      await this.transportWrapper.disconnect()
    }
    this.transportWrapper = new TransportWrapper()

    try {
      await this.transportWrapper.connect()
      this.transportWrapper.transport.on('disconnect', this.onDisconnected)

      let provider = new LedgerProvider({
        transport: this.transportWrapper.transport,
        minLedgerVersion: {
          major: 0,
          minor: 0,
          patch: 1
        }
      })

      if (!await provider.ready()) {
        return false
      }

      let accounts = await provider.getAccounts(0, 1, CoinType.TEST)
      this.deviceId = accounts[0]

      this.provider = provider

      return true
    } catch (e) {
      return false
    }
  }

  private onDisconnected = (e: any) => {
    if (e.name !== 'DisconnectedDevice') {
      return
    }
    this.provider = undefined
    this.transportWrapper = undefined
  }
}
