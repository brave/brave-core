/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import { TransportStatusError } from '@ledgerhq/errors'
import Eth from '@ledgerhq/hw-app-eth'
import {
  EthGetAccountCommand,
  EthGetAccountResponse,
  EthSignEip712MessageCommand,
  EthSignEip712MessageResponse,
  EthSignPersonalMessageCommand,
  EthSignPersonalMessageResponse,
  EthSignTransactionCommand,
  EthSignTransactionResponse,
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'
import { Untrusted } from '../untrusted_shared_types'

function vToHex(vNumber: number) {
  let v = vNumber < 27 ? vNumber.toString(16) : (vNumber - 27).toString(16)

  // Pad v with a leading zero if value is under `16` (i.e., single character
  // hex).
  if (v.length < 2) {
    v = `0${v}`
  }
  return v
}

function createEthereumSignatureVRS(
  v: string,
  r: string,
  s: string
): Untrusted.EthereumSignatureVRS {
  return {
    vBytes: Buffer.from(v, 'hex'),
    rBytes: Buffer.from(r, 'hex'),
    sBytes: Buffer.from(s, 'hex')
  }
}

function createEthereumSignatureBytes(
  v: string,
  r: string,
  s: string
): Untrusted.EthereumSignatureBytes {
  return {
    bytes: Buffer.concat([
      Buffer.from(r, 'hex'),
      Buffer.from(s, 'hex'),
      Buffer.from(v, 'hex')
    ])
  }
}

/** makes calls to the Ethereum app on a Ledger device */
export class EthereumLedgerUntrustedMessagingTransport //
  extends LedgerUntrustedMessagingTransport
{
  constructor(targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(
      LedgerCommand.Unlock,
      this.handleUnlock
    )
    this.addCommandHandler<EthGetAccountResponse>(
      LedgerCommand.GetAccount,
      this.handleGetAccount
    )
    this.addCommandHandler<EthSignTransactionResponse>(
      LedgerCommand.SignTransaction,
      this.handleSignTransaction
    )
    this.addCommandHandler<EthSignPersonalMessageResponse>(
      LedgerCommand.SignPersonalMessage,
      this.handleSignPersonalMessage
    )
    this.addCommandHandler<EthSignEip712MessageResponse>(
      LedgerCommand.SignEip712Message,
      this.handleSignEip712Message
    )
  }

  private handleGetAccount = async (
    command: EthGetAccountCommand
  ): Promise<EthGetAccountResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.getAddress(command.path)
      const response: EthGetAccountResponse = {
        ...command,
        payload: {
          success: true,
          publicKey: result.publicKey,
          address: result.address,
          chainCode: result.chainCode
        }
      }
      return response
    } catch (error) {
      const response: EthGetAccountResponse = {
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
    command: EthSignTransactionCommand
  ): Promise<EthSignTransactionResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.signTransaction(command.path, command.rawTxHex)
      const response: EthSignTransactionResponse = {
        ...command,
        payload: {
          // https://github.com/LedgerHQ/ledger-live/tree/develop/libs/ledgerjs/packages/hw-app-eth#examples-2
          success: true,
          signature: createEthereumSignatureVRS(result.v, result.r, result.s)
        }
      }
      return response
    } catch (error) {
      const response: EthSignTransactionResponse = {
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

  private handleSignPersonalMessage = async (
    command: EthSignPersonalMessageCommand
  ): Promise<EthSignPersonalMessageResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.signPersonalMessage(
        command.path,
        command.messageHex
      )
      const response: EthSignPersonalMessageResponse = {
        ...command,
        payload: {
          success: true,
          signature: createEthereumSignatureBytes(
            // https://github.com/LedgerHQ/ledger-live/tree/develop/libs/ledgerjs/packages/hw-app-eth#examples-4
            vToHex(result.v),
            result.r,
            result.s
          )
        }
      }
      return response
    } catch (error) {
      const response: EthSignPersonalMessageResponse = {
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

  private handleSignEip712Message = async (
    command: EthSignEip712MessageCommand
  ): Promise<EthSignEip712MessageResponse> => {
    const transport = await TransportWebHID.create()
    const app = new Eth(transport)
    try {
      const result = await app.signEIP712HashedMessage(
        command.path,
        command.domainSeparatorHex,
        command.hashStructMessageHex
      )
      const response: EthSignEip712MessageResponse = {
        ...command,
        payload: {
          success: true,
          signature: createEthereumSignatureBytes(
            // https://github.com/LedgerHQ/ledger-live/tree/develop/libs/ledgerjs/packages/hw-app-eth#examples-5
            vToHex(result.v),
            result.r,
            result.s
          )
        }
      }
      return response
    } catch (error) {
      const response: EthSignEip712MessageResponse = {
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
