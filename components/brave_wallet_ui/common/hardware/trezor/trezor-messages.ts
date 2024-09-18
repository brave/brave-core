// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { loadTimeData } from '../../../../common/loadTimeData'
import { Untrusted } from '../untrusted_shared_types'
import {
  Unsuccessful,
  EthereumSignTransaction,
  CommonParams,
  Success,
  EthereumSignMessage,
  EthereumSignTypedHash
} from './trezor-connect-types'
export const kTrezorBridgeUrl = loadTimeData.getString(
  'braveWalletTrezorBridgeUrl'
)

export enum TrezorCommand {
  Unlock = 'trezor-unlock',
  GetAccounts = 'trezor-get-accounts',
  SignTransaction = 'trezor-sign-transaction',
  SignMessage = 'trezor-sign-message',
  SignTypedMessage = 'trezor-sign-typed-message'
}

export enum TrezorErrorsCodes {
  BridgeNotReady = 0,
  CommandInProgress = 1
}

export type CommandMessage = {
  command: TrezorCommand
  id: string
  origin: string
}
export type TrezorAccountPath = {
  path: string
}
export type TrezorAccount = {
  publicKey: string
  serializedPath: string
  fingerprint: number
}

// Unlock command
export type UnlockResponse =
  | Unsuccessful
  | {
      success: boolean
    }
export type UnlockResponsePayload = CommandMessage & {
  payload: UnlockResponse
}
export type UnlockCommand = CommandMessage & {
  command: TrezorCommand.Unlock
}

// GetAccounts command
export type TrezorGetAccountsResponse = Unsuccessful | Success<TrezorAccount[]>
export type GetAccountsResponsePayload = CommandMessage & {
  payload: TrezorGetAccountsResponse
}
export type GetAccountsCommand = CommandMessage & {
  command: TrezorCommand.GetAccounts
  paths: TrezorAccountPath[]
}

// SignTransaction command
export type SignTransactionCommandPayload = CommonParams &
  EthereumSignTransaction
export type SignTransactionCommand = CommandMessage & {
  command: TrezorCommand.SignTransaction
  payload: SignTransactionCommandPayload
}
export type SignTransactionResponse =
  | Unsuccessful
  | Success<Untrusted.EthereumSignatureVRS>
export type SignTransactionResponsePayload = CommandMessage & {
  payload: SignTransactionResponse
}

// SignMessage command
export type SignMessageCommandPayload = CommonParams & EthereumSignMessage
export type SignMessageCommand = CommandMessage & {
  command: TrezorCommand.SignMessage
  payload: SignMessageCommandPayload
}
export type SignMessageResponse =
  | Unsuccessful
  | Success<Untrusted.EthereumSignatureBytes>
export type SignMessageResponsePayload = CommandMessage & {
  payload: SignMessageResponse
}

// SignTypedMessage command
export type SignTypedMessageCommandPayload = CommonParams &
  EthereumSignTypedHash
export type SignTypedMessageCommand = CommandMessage & {
  command: TrezorCommand.SignTypedMessage
  payload: SignTypedMessageCommandPayload
}
export type SignTypedMessageResponse =
  | Unsuccessful
  | Success<Untrusted.EthereumSignatureBytes>
export type SignTypedMessageResponsePayload = CommandMessage & {
  payload: SignTypedMessageResponse
}

export type TrezorFrameCommand =
  | GetAccountsCommand
  | UnlockCommand
  | SignTransactionCommand
  | SignMessageCommand
  | SignTypedMessageCommand
export type TrezorFrameResponse =
  | UnlockResponsePayload
  | GetAccountsResponsePayload
  | SignTransactionResponsePayload
  | SignMessageResponsePayload

// Trezor library is loaded inside the chrome-untrusted webui page
// and communication is going through posting messages between parent window
// and frame window. This class handles low level messages transport to add,
// remove callbacks and allows to process messages for childrens.
export abstract class MessagingTransport {
  constructor() {
    this.handlers = new Map<string, Function>()
  }

  protected handlers: Map<string, Function>

  addCommandHandler = (id: string, listener: Function): boolean => {
    if (!this.handlers.size) {
      this.addWindowMessageListener()
      this.handlers.clear()
    }
    if (this.handlers.has(id)) {
      return false
    }
    this.handlers.set(id, listener)
    return true
  }

  protected removeCommandHandler = (id: string) => {
    if (!this.handlers.has(id)) {
      return false
    }
    this.handlers.delete(id)
    if (!this.handlers.size) {
      this.removeWindowMessageListener()
    }
    return true
  }

  protected abstract onMessageReceived(event: MessageEvent): unknown

  private readonly addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived)
  }

  private readonly removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived)
  }
}
