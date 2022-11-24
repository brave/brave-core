/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Eth from '@ledgerhq/hw-app-eth'
import {
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import {
  EthGetAccountCommand,
  EthGetAccountResponse,
  EthGetAccountResponsePayload,
  EthSignTransactionCommand,
  EthSignTransactionResponsePayload,
  EthSignTransactionResponse,
  EthSignPersonalMessageCommand,
  EthSignPersonalMessageResponsePayload,
  EthSignPersonalMessageResponse,
  EthSignEip712MessageCommand,
  EthSignEip712MessageResponsePayload,
  EthSignEip712MessageResponse
} from './eth-ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'

// EthereumLedgerUntrustedMessagingTransport makes calls to the Ethereum app on a Ledger device
export class EthereumLedgerUntrustedMessagingTransport extends LedgerUntrustedMessagingTransport {
  constructor (targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(LedgerCommand.Unlock, this.handleUnlock)
    this.addCommandHandler<EthGetAccountResponse>(LedgerCommand.GetAccount, this.handleGetAccount)
    this.addCommandHandler<EthSignTransactionResponse>(LedgerCommand.SignTransaction, this.handleSignTransaction)
    this.addCommandHandler<EthSignPersonalMessageResponse>(LedgerCommand.SignPersonalMessage, this.handleSignPersonalMessage)
    this.addCommandHandler<EthSignEip712MessageResponse>(LedgerCommand.SignEip712Message, this.handleSignEip712Message)
  }

  private handleGetAccount = async (command: EthGetAccountCommand): Promise<EthGetAccountResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.getAddress(command.path)
      const getAccountResponsePayload: EthGetAccountResponsePayload = {
        success: true,
        publicKey: result.publicKey,
        address: result.address,
        chainCode: result.chainCode
      }
      const response: EthGetAccountResponse = {
        id: command.id,
        command: command.command,
        payload: getAccountResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: EthGetAccountResponse = {
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

  private handleSignTransaction = async (command: EthSignTransactionCommand): Promise<EthSignTransactionResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.signTransaction(command.path, command.rawTxHex)
      const signTransactionResponsePayload: EthSignTransactionResponsePayload = {
        success: true,
        v: result.v,
        r: result.r,
        s: result.s
      }
      const response: EthSignTransactionResponse = {
        id: command.id,
        command: command.command,
        payload: signTransactionResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: EthSignTransactionResponse = {
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

  private handleSignPersonalMessage = async (command: EthSignPersonalMessageCommand): Promise<EthSignPersonalMessageResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.signPersonalMessage(command.path, command.messageHex)
      const signPersonalMessageResponsePayload: EthSignPersonalMessageResponsePayload = {
        success: true,
        v: result.v,
        r: result.r,
        s: result.s
      }
      const response: EthSignPersonalMessageResponse = {
        id: command.id,
        command: command.command,
        payload: signPersonalMessageResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: EthSignPersonalMessageResponse = {
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

  private handleSignEip712Message = async (command: EthSignEip712MessageCommand): Promise<EthSignEip712MessageResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.signEIP712HashedMessage(command.path, command.domainSeparatorHex, command.hashStructMessageHex)
      const signEip712MessageResponsePayload: EthSignEip712MessageResponsePayload = {
        success: true,
        v: result.v,
        r: result.r,
        s: result.s
      }
      const response: EthSignEip712MessageResponse = {
        id: command.id,
        command: command.command,
        payload: signEip712MessageResponsePayload,
        origin: command.origin
      }
      return response
    } catch (error) {
      const response: EthSignEip712MessageResponse = {
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
