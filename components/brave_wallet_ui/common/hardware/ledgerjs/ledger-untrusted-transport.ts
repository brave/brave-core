/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Sol from '@ledgerhq/hw-app-solana'
import { LedgerMessagingTransport } from './ledger-messaging-transport'
import { HardwareOperationResult } from '../../hardware/types'
import {
  LedgerCommand,
  LedgerResponsePayload,
  UnlockCommand,
  UnlockResponse,
  GetAccountCommand,
  GetAccountResponse,
  GetAccountResponsePayload,
  SignTransactionCommand,
  SignTransactionResponsePayload,
  SignTransactionResponse
} from './ledger-messages'

// LedgerUntrustedMessagingTransport is the messaging transport object
// for chrome-untrusted://ledger-bridge. It primarily handles postMessages
// coming from chrome://wallet or chrome://wallet-panel by making calls
// to Ledger libraries and replying with the results.
//
// We isolate the Ledger library from the wallet
// so that in the event it's compromised it will reduce the
// impact to the wallet.
export class LedgerUntrustedMessagingTransport extends LedgerMessagingTransport {
  constructor (targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(LedgerCommand.Unlock, this.handleUnlock)
    this.addCommandHandler<GetAccountResponse>(LedgerCommand.GetAccount, this.handleGetAccount)
    this.addCommandHandler<SignTransactionResponse>(LedgerCommand.SignTransaction, this.handleSignTransaction)
  }

  promptAuthorization = async () => {
    if (await this.authorizationNeeded()) {
      const transport = await TransportWebHID.create()
      await transport.close()
    }
  }

  private handleUnlock = async (command: UnlockCommand): Promise<UnlockResponse> => {
    const isAuthNeeded = await this.authorizationNeeded()
    const payload: LedgerResponsePayload | HardwareOperationResult =
      isAuthNeeded
        ? { success: false, error: 'unauthorized', code: 'unauthorized' }
        : { success: true }

    const responsePayload: UnlockResponse = {
      id: command.id,
      command: command.command,
      payload: payload,
      origin: command.origin
    }
    return responsePayload
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

  private authorizationNeeded = async (): Promise<boolean> => {
    return (await TransportWebHID.list()).length === 0
  }
}
