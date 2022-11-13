/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { FilecoinNetwork } from '../types'
import type {
  CommandMessage,
  LedgerCommand,
  LedgerError,
  LedgerResponsePayload
} from './ledger-messages'

// GetAccounts command
export type FilGetAccountResponsePayload = LedgerResponsePayload & {
  accounts: string[]
  deviceId: string
}

export type FilGetAccountResponse = CommandMessage & {
  payload: FilGetAccountResponsePayload | LedgerError
}

export type FilGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  from: number
  to: number
  network: FilecoinNetwork
}

export interface FilLotusMessage {
  To: string
  From: string
  Nonce: number
  Value: string
  GasPremium: string
  GasLimit: number
  GasFeeCap: string
  Method: number
  Params?: string | string[]
}

export interface FilSignedLotusMessage {
  Message: FilLotusMessage
  Signature: {
      Type: number
      Data: string
  }
}

// SignTransaction command
export type FilSignedTx = {
  lotusMessage: FilSignedLotusMessage
}
export type FilSignTransactionResponsePayload = LedgerResponsePayload & FilSignedTx

export type FilSignTransactionResponse = CommandMessage & {
  payload: FilSignTransactionResponsePayload | LedgerError
}

export type FilSignTransactionCommand = CommandMessage & {
  command: LedgerCommand.SignTransaction
  message: string
}

export type FilLedgerFrameCommand = FilGetAccountCommand | FilSignTransactionCommand
export type FilLedgerFrameResponse = FilGetAccountResponse | FilSignTransactionResponse
