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
export type EthGetAccountResponsePayload = LedgerResponsePayload & {
  publicKey: string
  address: string
  chainCode?: string
}

export type EthGetAccountResponse = CommandMessage & {
  payload: EthGetAccountResponsePayload | LedgerError
}

export type EthGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  path: string
}

// SignTransaction command
export type EthereumSignedTx = {
  v: string
  r: string
  s: string
}
export type EthSignTransactionResponsePayload = LedgerResponsePayload & EthereumSignedTx

export type EthSignTransactionResponse = CommandMessage & {
  payload: EthSignTransactionResponsePayload | LedgerError
}

export type EthSignTransactionCommand = CommandMessage & {
  command: LedgerCommand.SignTransaction
  path: string
  rawTxHex: string
}

// SignPersonalMessage command
export type EthSignPersonalMessageResponsePayload = LedgerResponsePayload & {
  v: number
  r: string
  s: string
}

export type EthSignPersonalMessageResponse = CommandMessage & {
  payload: EthSignPersonalMessageResponsePayload | LedgerError
}

export type EthSignPersonalMessageCommand = CommandMessage & {
  command: LedgerCommand.SignPersonalMessage
  path: string
  messageHex: string
}

// SignEip712Message command
export type EthSignEip712MessageResponsePayload = EthSignPersonalMessageResponsePayload

export type EthSignEip712MessageResponse = CommandMessage & {
  payload: EthSignEip712MessageResponsePayload | LedgerError
}

export type EthSignEip712MessageCommand = CommandMessage & {
  command: LedgerCommand.SignEip712Message
  path: string
  domainSeparatorHex: string
  hashStructMessageHex: string
}

export type EthLedgerFrameCommand = EthGetAccountCommand | EthSignTransactionCommand | EthSignPersonalMessageCommand | EthSignEip712MessageCommand
export type EthLedgerFrameResponse = EthGetAccountResponse | EthSignTransactionResponse | EthSignPersonalMessageResponse | EthSignEip712MessageResponse
