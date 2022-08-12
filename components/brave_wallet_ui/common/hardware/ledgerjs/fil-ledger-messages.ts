/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { SignedLotusMessage } from '@glif/filecoin-message'
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

// SignTransaction command
export type FilSignedTx = {
  lotusMessage: SignedLotusMessage
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
