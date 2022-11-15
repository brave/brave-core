/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Sol from '@ledgerhq/hw-app-solana'
import {
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import {
  SolGetAccountCommand,
  SolGetAccountResponse,
  SolGetAccountResponsePayload,
  SolSignTransactionCommand,
  SolSignTransactionResponsePayload,
  SolSignTransactionResponse
} from './sol-ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

// SolanaLedgerUntrustedMessagingTransport makes calls to the Solana app on a Ledger device
export class SolanaLedgerUntrustedMessagingTransport extends LedgerUntrustedMessagingTransport {
  constructor (targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(LedgerCommand.Unlock, this.handleUnlock)
    this.addCommandHandler<SolGetAccountResponse>(LedgerCommand.GetAccount, this.handleGetAccount)
    this.addCommandHandler<SolSignTransactionResponse>(LedgerCommand.SignTransaction, this.handleSignTransaction)
  }

  private handleGetAccount = async (command: SolGetAccountCommand): Promise<SolGetAccountResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Sol(transport)
    try {
      const result = await app.getAddress(command.path)
      const getAccountResponsePayload: SolGetAccountResponsePayload = {
        success: true,
        address: result.address
      }
      const response: SolGetAccountResponse = {
        id: command.id,
        command: command.command,
        payload: getAccountResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: SolGetAccountResponse = {
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

  private handleSignTransaction = async (command: SolSignTransactionCommand): Promise<SolSignTransactionResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Sol(transport)
    try {
      const result = await app.signTransaction(command.path, Buffer.from(command.rawTxBytes))
      const signTransactionResponsePayload: SolSignTransactionResponsePayload = {
        success: true,
        signature: result.signature
      }
      const response: SolSignTransactionResponse = {
        id: command.id,
        command: command.command,
        payload: signTransactionResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: SolSignTransactionResponse = {
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
