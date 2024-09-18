/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Sol from '@ledgerhq/hw-app-solana'
import { TransportStatusError } from '@ledgerhq/errors'
import {
  LedgerCommand,
  UnlockResponse,
  SolGetAccountCommand,
  SolGetAccountResponse,
  SolSignTransactionCommand,
  SolSignTransactionResponse
} from './ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

/** makes calls to the Solana app on a Ledger device */
export class SolanaLedgerUntrustedMessagingTransport //
  extends LedgerUntrustedMessagingTransport
{
  constructor(targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(
      LedgerCommand.Unlock,
      this.handleUnlock
    )
    this.addCommandHandler<SolGetAccountResponse>(
      LedgerCommand.GetAccount,
      this.handleGetAccount
    )
    this.addCommandHandler<SolSignTransactionResponse>(
      LedgerCommand.SignTransaction,
      this.handleSignTransaction
    )
  }

  private handleGetAccount = async (
    command: SolGetAccountCommand
  ): Promise<SolGetAccountResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Sol(transport)
    try {
      const result = await app.getAddress(command.path)
      const response: SolGetAccountResponse = {
        ...command,
        payload: { success: true, address: result.address }
      }
      return response
    } catch (error) {
      const response: SolGetAccountResponse = {
        ...command,
        payload: {
          success: false,
          error: (error as Error).message,
          code:
            error instanceof TransportStatusError ? error.statusCode : undefined
        }
      }
      return response
    } finally {
      await transport.close()
    }
  }

  private handleSignTransaction = async (
    command: SolSignTransactionCommand
  ): Promise<SolSignTransactionResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Sol(transport)
    try {
      const result = await app.signTransaction(
        command.path,
        Buffer.from(command.rawTxBytes)
      )

      const response: SolSignTransactionResponse = {
        ...command,
        payload: {
          success: true,
          untrustedSignatureBytes: result.signature
        }
      }
      return response
    } catch (error) {
      const response: SolSignTransactionResponse = {
        ...command,
        payload: {
          success: false,
          error: (error as Error).message,
          code:
            error instanceof TransportStatusError ? error.statusCode : undefined
        }
      }
      return response
    } finally {
      await transport.close()
    }
  }
}
