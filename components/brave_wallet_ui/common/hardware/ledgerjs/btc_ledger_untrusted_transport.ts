/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Transport from '@ledgerhq/hw-transport'
import { TransportStatusError } from '@ledgerhq/errors'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import Btc from '@ledgerhq/hw-app-btc'
import { BufferReader, BufferWriter } from '@ledgerhq/hw-app-btc/buffertools'
import {
  BtcGetAccountCommand,
  BtcGetAccountResponse,
  BtcSignTransactionCommand,
  BtcSignTransactionResponse,
  LedgerCommand,
  UnlockResponse
} from './ledger-messages'
import { LedgerUntrustedMessagingTransport } from './ledger-untrusted-transport'
import { CreateTransactionArg } from '@ledgerhq/hw-app-btc/lib/createTransaction'

/** makes calls to the Btc app on a Ledger device */
export class BitcoinLedgerUntrustedMessagingTransport //
  extends LedgerUntrustedMessagingTransport
{
  constructor(targetWindow: Window, targetUrl: string) {
    super(targetWindow, targetUrl)
    this.addCommandHandler<UnlockResponse>(
      LedgerCommand.Unlock,
      this.handleUnlock
    )
    this.addCommandHandler<BtcGetAccountResponse>(
      LedgerCommand.GetAccount,
      this.handleGetAccount
    )
    this.addCommandHandler<BtcSignTransactionResponse>(
      LedgerCommand.SignTransaction,
      this.handleSignTransaction
    )
  }

  private handleGetAccount = async (
    command: BtcGetAccountCommand
  ): Promise<BtcGetAccountResponse> => {
    let transport: Transport | undefined
    try {
      transport = await TransportWebHID.create()
      const app = new Btc({ transport })
      const result = await app.getWalletXpub({
        path: command.path,
        xpubVersion: command.xpubVersion
      })
      const response: BtcGetAccountResponse = {
        ...command,
        payload: { success: true, xpub: result }
      }
      return response
    } catch (error) {
      const response: BtcGetAccountResponse = {
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
      await transport?.close()
    }
  }

  private handleSignTransaction = async (
    command: BtcSignTransactionCommand
  ): Promise<BtcSignTransactionResponse> => {
    let transport: Transport | undefined

    try {
      transport = await TransportWebHID.create()
      const app = new Btc({ transport })
      const signedTransactionHex = await app.createPaymentTransaction({
        inputs: command.inputTransactions.map((i) => {
          return [
            app.splitTransaction(Buffer.from(i.txBytes).toString('hex'), true),
            i.outputIndex,
            undefined,
            0xfffffffd // sequence number same as for core transactions.
          ]
        }),
        associatedKeysets: command.inputTransactions.map(
          (i) => i.associatedPath
        ),
        outputScriptHex: Buffer.from(command.outputScript).toString('hex'),
        additionals: ['bech32'],
        changePath: command.changePath,
        lockTime: command.lockTime,
        sigHashType: 1, // SIGHASH_ALL
        segwit: true
      } satisfies CreateTransactionArg)

      const signedTransaction = app.splitTransaction(signedTransactionHex, true)

      if (!signedTransaction.witness) {
        throw new Error('Unexpected empty witness.')
      }

      const witnesses: Uint8Array[] = []
      const witnessesReader = new BufferReader(signedTransaction.witness)
      while (!witnessesReader.available) {
        const witnessField = witnessesReader.readVector()
        if (witnessField.length !== 2) {
          throw new Error('Invalid witness field size.')
        }
        const witnessBuf = new BufferWriter()
        witnessBuf.writeVarInt(2)
        witnessBuf.writeVarSlice(witnessField[0])
        witnessBuf.writeVarSlice(witnessField[1])
        witnesses.push(witnessBuf.buffer())
      }

      if (witnesses.length !== command.inputTransactions.length) {
        throw new Error('Invalid number of witness fields.')
      }

      const response: BtcSignTransactionResponse = {
        ...command,
        payload: {
          success: true,
          witnesses
        }
      }
      return response
    } catch (error) {
      const response: BtcSignTransactionResponse = {
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
      await transport?.close()
    }
  }
}
