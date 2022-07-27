/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Sol from '@ledgerhq/hw-app-solana'
import {
  LedgerCommand,
  UnlockResponse,
  GetAccountCommand,
  GetAccountResponse,
  GetAccountResponsePayload,
  SignTransactionCommand,
  SignTransactionResponsePayload,
  SignTransactionResponse
} from './ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

// SolanaLedgerUntrustedMessagingTransport makes calls to the Solana app on a Ledger device
export class SolanaLedgerUntrustedMessagingTransport extends LedgerUntrustedMessagingTransport {
  constructor (targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(LedgerCommand.Unlock, this.handleUnlock)
    this.addCommandHandler<GetAccountResponse>(LedgerCommand.GetAccount, this.handleGetAccount)
    this.addCommandHandler<SignTransactionResponse>(LedgerCommand.SignTransaction, this.handleSignTransaction)
  }

  private handleGetAccount = async (command: GetAccountCommand): Promise<GetAccountResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Sol(transport)
    try {
      const result = await app.getAddress(command.path)
      const getAccountResponsePayload: GetAccountResponsePayload = {
        success: true,
        address: result.address
      }
      const response: GetAccountResponse = {
        id: command.id,
        command: command.command,
        payload: getAccountResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: GetAccountResponse = {
        id: command.id,
        command: command.command,
        payload: error,
        origin: command.origin
      }
      return response
    } finally {
      await transport.close()
    }
  }

  private handleSignTransaction = async (command: SignTransactionCommand): Promise<SignTransactionResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Sol(transport)
    try {
      const result = await app.signTransaction(command.path, Buffer.from(command.rawTxBytes))
      const signTransactionResponsePayload: SignTransactionResponsePayload = {
        success: true,
        signature: result.signature
      }
      const response: SignTransactionResponse = {
        id: command.id,
        command: command.command,
        payload: signTransactionResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: SignTransactionResponse = {
        id: command.id,
        command: command.command,
        payload: error,
        origin: command.origin
      }
      return response
    } finally {
      await transport.close()
    }
  }
}
