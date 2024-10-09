/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '../../../../common/loadTimeData'
import { Untrusted } from '../untrusted_shared_types'

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

export type LedgerError = {
  error: string
  code: number | string | undefined
}

export type LedgerResponse<T extends object = {}> =
  | ({ success: true } & T)
  | ({ success: false } & LedgerError)

export type LedgerResponsePayload<T extends object = {}> = {
  payload: LedgerResponse<T>
}

// Unlock command
// Note: Relative to the Trezor messages, *Response and *ResponsePayload
// are reversed. The Ledger *Response messages will have a payload field
// of type *ResponsePayload, whereas Trezor will have a *ResponsePayload
// messages with a payload field of type *Response.
export type UnlockResponse = CommandMessage & LedgerResponsePayload
export type UnlockCommand = CommandMessage & {
  command: LedgerCommand.Unlock
}

// AuthorizationRequired command
export type AuthorizationRequiredCommand = CommandMessage & {
  command: LedgerCommand.AuthorizationRequired
}
export type AuthorizationSuccessResponsePayload = CommandMessage &
  LedgerResponsePayload
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
export type SolGetAccountResponse = CommandMessage &
  LedgerResponsePayload<{ address: Uint8Array }>
export type SolGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  path: string
}

// SignTransaction command
export type SolSignTransactionResponse = CommandMessage &
  LedgerResponsePayload<{
    untrustedSignatureBytes: Uint8Array
  }>
export type SolSignTransactionCommand = CommandMessage & {
  command: LedgerCommand.SignTransaction
  path: string
  rawTxBytes: Uint8Array
}

export type SolLedgerFrameCommand =
  | SolGetAccountCommand
  | SolSignTransactionCommand
export type SolLedgerFrameResponse =
  | SolGetAccountResponse
  | SolSignTransactionResponse

// Filecoin
// GetAccounts command
export type FilGetAccountResponse = CommandMessage &
  LedgerResponsePayload<{
    accounts: string[]
  }>
export type FilGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  from: number
  count: number
  isTestnet: boolean
}

// SignTransaction command
export type FilSignTransactionResponse = CommandMessage &
  LedgerResponsePayload<{ untrustedSignedTxJson: string }>
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
  inputTransactions: Array<{
    txBytes: Uint8Array
    outputIndex: number
    associatedPath: string
  }>
  outputScript: Uint8Array
  changePath: string | undefined
  lockTime: number
}

export type BtcGetAccountResponse = CommandMessage &
  LedgerResponsePayload<{ xpub: string }>
export type BtcSignTransactionResponse = CommandMessage &
  LedgerResponsePayload<{ witnesses: Uint8Array[] }>

export type BtcLedgerFrameCommand =
  | BtcGetAccountCommand
  | BtcSignTransactionCommand
export type BtcLedgerFrameResponse =
  | BtcGetAccountResponse
  | BtcSignTransactionResponse

// Ethereum
// GetAccounts command
export type EthGetAccountResponse = CommandMessage &
  LedgerResponsePayload<{
    publicKey: string
    address: string
    chainCode?: string
  }>
export type EthGetAccountCommand = CommandMessage & {
  command: LedgerCommand.GetAccount
  path: string
}

// SignTransaction command
export type EthSignTransactionResponse = CommandMessage &
  LedgerResponsePayload<{ signature: Untrusted.EthereumSignatureVRS }>
export type EthSignTransactionCommand = CommandMessage & {
  command: LedgerCommand.SignTransaction
  path: string
  rawTxHex: string
}

// SignPersonalMessage command
export type EthSignPersonalMessageResponse = CommandMessage &
  LedgerResponsePayload<{ signature: Untrusted.EthereumSignatureBytes }>
export type EthSignPersonalMessageCommand = CommandMessage & {
  command: LedgerCommand.SignPersonalMessage
  path: string
  messageHex: string
}

// SignEip712Message command
export type EthSignEip712MessageResponse = CommandMessage &
  LedgerResponsePayload<{ signature: Untrusted.EthereumSignatureBytes }>
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
