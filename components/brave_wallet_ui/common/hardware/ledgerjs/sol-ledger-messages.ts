/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import type {
  CommandMessage,
  LedgerCommand,
  LedgerError,
  LedgerResponsePayload
} from './ledger-messages'

// GetAccounts command
export type SolGetAccountResponsePayload = LedgerResponsePayload & {
  address: Buffer
}
export type SolGetAccountResponse = CommandMessage & {
  payload: SolGetAccountResponsePayload | LedgerError
}
export type SolGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  path: string
}

// SignTransaction command
export type SolSignTransactionResponsePayload = LedgerResponsePayload & {
  signature: Buffer
}
export type SolSignTransactionResponse = CommandMessage & {
  payload: SolSignTransactionResponsePayload | LedgerError
}
export type SolSignTransactionCommand = CommandMessage & {
  command: LedgerCommand.SignTransaction
  path: string
  rawTxBytes: Buffer
}

export type SolLedgerFrameCommand = SolGetAccountCommand | SolSignTransactionCommand
export type SolLedgerFrameResponse = SolGetAccountResponse | SolSignTransactionResponse
