// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import { loadTimeData } from '../../../common/loadTimeData'
import { Unsuccessful, EthereumSignTransaction, CommonParams, Success, EthereumSignMessage } from 'trezor-connect'
import { EthereumSignedTx, HDNodeResponse, MessageSignature } from 'trezor-connect/lib/typescript/trezor/protobuf'
export const kTrezorBridgeUrl = loadTimeData.getString('braveWalletTrezorBridgeUrl')

export enum TrezorCommand {
  Unlock = 'trezor-unlock',
  GetAccounts = 'trezor-get-accounts',
  SignTransaction = 'trezor-sign-treansaction',
  SignMessage = 'trezor-sign-message'
}
export type CommandMessage = {
  command: TrezorCommand
  id: string
  origin: string
}
export type UnlockCommand = CommandMessage & {
  command: TrezorCommand.Unlock
}
export type UnlockResponse = CommandMessage & {
  result: Boolean,
  error?: Unsuccessful
}
export type TrezorAccountPath = {
  path: string
}
export type TrezorAccount = {
  publicKey: string
  serializedPath: string,
  fingerprint: number
}
export type TrezorError = {
  error: string,
  code?: string | number
}
export type TrezorGetPublicKeyResponse = Unsuccessful | Success<HDNodeResponse[]>
export type GetAccountsResponsePayload = CommandMessage & {
  payload: TrezorGetPublicKeyResponse
}
export type GetAccountsCommand = CommandMessage & {
  command: TrezorCommand.GetAccounts,
  paths: TrezorAccountPath[]
}

export type SignTransactionCommandPayload = CommonParams & EthereumSignTransaction
export type SignTransactionCommand = CommandMessage & {
  command: TrezorCommand.SignTransaction
  payload: SignTransactionCommandPayload
}
export type SignTransactionResponse = Unsuccessful | Success<EthereumSignedTx>
export type SignTransactionResponsePayload = CommandMessage & {
  payload: SignTransactionResponse
}

export type SignMessageCommandPayload = CommonParams & EthereumSignMessage
export type SignMessageCommand = CommandMessage & {
  command: TrezorCommand.SignMessage
  payload: SignMessageCommandPayload
}
export type SignMessageResponse = Unsuccessful | Success<MessageSignature>
export type SignMessageResponsePayload = CommandMessage & {
  payload: SignMessageResponse
}

export type TrezorFrameCommand = GetAccountsCommand | UnlockCommand | SignTransactionCommand | SignMessageCommand
export type TrezorFrameResponse = UnlockResponse | GetAccountsResponsePayload | SignTransactionResponsePayload | SignMessageResponsePayload

// Trezor library is loaded inside the chrome-untrusted webui page
// and communication is going through posting messages between parent window
// and frame window. This class handles low level messages transport to add,
// remove callbacks and allows to process messages for childrens.
export abstract class MessagingTransport {
  constructor () {
    this.handlers = new Map<string, Function>()
  }

  protected handlers: Map<string, Function>

  addCommandHandler = (id: string, listener: Function): Boolean => {
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

  protected abstract onMessageReceived (event: MessageEvent): unknown

  private addWindowMessageListener = () => {
    window.addEventListener('message', this.onMessageReceived)
  }

  private removeWindowMessageListener = () => {
    window.removeEventListener('message', this.onMessageReceived)
  }
}
