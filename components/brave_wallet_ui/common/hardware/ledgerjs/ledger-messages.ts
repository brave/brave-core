/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '../../../../common/loadTimeData'

const braveWalletLedgerBridgeUrl = loadTimeData.getString(
  'braveWalletLedgerBridgeUrl'
)
export const LEDGER_BRIDGE_URL =
  braveWalletLedgerBridgeUrl.charAt(braveWalletLedgerBridgeUrl.length - 1) ===
  '/'
    ? braveWalletLedgerBridgeUrl.slice(0, -1) // Strip off trailing '/' in URL
    : braveWalletLedgerBridgeUrl
export enum LedgerCommand {
  Unlock = 'ledger-unlock',
  GetAccount = 'ledger-get-accounts',
  SignTransaction = 'ledger-sign-transaction',
  SignPersonalMessage = 'ledger-sign-personal-message',
  SignEip712Message = 'ledger-sign-eip-712-message',
  AuthorizationRequired = 'authorization-required', // Sent by the frame to the parent context
  AuthorizationSuccess = 'authorization-success' // Sent by the frame to the parent context
}

// LedgerBridgeErrorCodes are errors related to the configuring
// and running of postMessages between window objects
export enum LedgerBridgeErrorCodes {
  BridgeNotReady = 0,
  CommandInProgress = 1
}

export type CommandMessage = {
  command: LedgerCommand
  id: string
  origin: string
}

export type LedgerResponsePayload = {
  success: boolean
}

export type LedgerError = LedgerResponsePayload & {
  message?: string
  statusCode?: number
  id?: string
  name?: string
}

// Unlock command
// Note: Relative to the Trezor messages, *Response and *ResponsePayload
// are reversed. The Ledger *Response messages will have a payload field
// of type *ResponsePayload, whereas Trezor will have a *ResponsePayload
// messages with a payload field of type *Response.
export type UnlockResponse = CommandMessage & {
  payload: LedgerResponsePayload | LedgerError
}
export type UnlockCommand = CommandMessage & {
  command: LedgerCommand.Unlock
}

// AuthorizationRequired command
export type AuthorizationRequiredCommand = CommandMessage & {
  command: LedgerCommand.AuthorizationRequired
}
export type AuthorizationSuccessResponsePayload = CommandMessage & {
  payload: LedgerResponsePayload
}
export type AuthorizationSuccessCommand = CommandMessage & {
  command: LedgerCommand.AuthorizationSuccess
}

export type LedgerFrameCommand =
  | UnlockCommand
  | AuthorizationRequiredCommand
  | AuthorizationSuccessCommand
  | SolLedgerFrameCommand
  | EthLedgerFrameCommand
  | FilLedgerFrameCommand
  | BtcLedgerFrameCommand
export type LedgerFrameResponse =
  | UnlockResponse
  | SolLedgerFrameResponse
  | EthLedgerFrameResponse
  | FilLedgerFrameResponse

type LedgerCommandHandler<T> = (command: LedgerFrameCommand) => Promise<T>
type LedgerCommandResponseHandler<T> = (response: T) => void
export type LedgerCommandHandlerUnion<T> =
  | LedgerCommandHandler<T>
  | LedgerCommandResponseHandler<T>

// Solana
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

export type SolLedgerFrameCommand =
  | SolGetAccountCommand
  | SolSignTransactionCommand
export type SolLedgerFrameResponse =
  | SolGetAccountResponse
  | SolSignTransactionResponse

// Filecoin
// GetAccounts command
export type FilGetAccountResponsePayload = LedgerResponsePayload & {
  accounts: string[]
}

export type FilGetAccountResponse = CommandMessage & {
  payload: FilGetAccountResponsePayload | LedgerError
}

export type FilGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  from: number
  count: number
  isTestnet: boolean
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
export type FilSignTransactionResponsePayload = LedgerResponsePayload &
  FilSignedTx

export type FilSignTransactionResponse = CommandMessage & {
  payload: FilSignTransactionResponsePayload | LedgerError
}

export type FilSignTransactionCommand = CommandMessage & {
  command: LedgerCommand.SignTransaction
  message: string
}

export type FilLedgerFrameCommand =
  | FilGetAccountCommand
  | FilSignTransactionCommand
export type FilLedgerFrameResponse =
  | FilGetAccountResponse
  | FilSignTransactionResponse

export type BtcGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  path: string
  xpubVersion: number
}
export type BtcSignTransactionCommand = CommandMessage & {
  command: LedgerCommand.SignTransaction
  path: string
  rawTxBytes: Buffer
}

export type BtcGetAccountResponsePayload = LedgerResponsePayload & {
  xpub: string
}
export type BtcGetAccountResponse = CommandMessage & {
  payload: BtcGetAccountResponsePayload | LedgerError
}
export type BtcSignTransactionResponsePayload = LedgerResponsePayload & {
  signature: Buffer
}
export type BtcSignTransactionResponse = CommandMessage & {
  payload: BtcSignTransactionResponsePayload | LedgerError
}

export type BtcLedgerFrameCommand =
  | BtcGetAccountCommand
  | BtcSignTransactionCommand
export type BtcLedgerFrameResponse =
  | BtcGetAccountResponse
  | BtcSignTransactionResponse

// Ethereum
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
export type EthSignTransactionResponsePayload = LedgerResponsePayload &
  EthereumSignedTx

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
export type EthSignEip712MessageResponsePayload =
  EthSignPersonalMessageResponsePayload

export type EthSignEip712MessageResponse = CommandMessage & {
  payload: EthSignEip712MessageResponsePayload | LedgerError
}

export type EthSignEip712MessageCommand = CommandMessage & {
  command: LedgerCommand.SignEip712Message
  path: string
  domainSeparatorHex: string
  hashStructMessageHex: string
}

export type EthLedgerFrameCommand =
  | EthGetAccountCommand
  | EthSignTransactionCommand
  | EthSignPersonalMessageCommand
  | EthSignEip712MessageCommand
export type EthLedgerFrameResponse =
  | EthGetAccountResponse
  | EthSignTransactionResponse
  | EthSignPersonalMessageResponse
  | EthSignEip712MessageResponse
