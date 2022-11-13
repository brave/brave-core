// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import TrezorConnect from 'trezor-connect'
import { Unsuccessful, Success } from '../common/hardware/trezor/trezor-connect-types'
import { EthereumSignedTx } from 'trezor-connect/lib/typescript/networks/ethereum'
import {
  TrezorCommand,
  UnlockCommand,
  UnlockResponsePayload,
  GetAccountsCommand,
  GetAccountsResponsePayload,
  TrezorGetAccountsResponse,
  SignTransactionCommand,
  SignTransactionResponsePayload,
  SignMessageCommand,
  SignMessageResponsePayload,
  SignMessageResponse,
  UnlockResponse,
  SignTypedMessageCommand,
  SignTypedMessageResponsePayload
} from '../common/hardware/trezor/trezor-messages'
import { addTrezorCommandHandler } from '../common/hardware/trezor/trezor-command-handler'

const createUnlockResponse = (command: UnlockCommand, result: boolean, error?: Unsuccessful): UnlockResponsePayload => {
  const payload: UnlockResponse = (!result && error) ? error : { success: result }
  return { id: command.id, command: command.command, payload: payload, origin: command.origin }
}

const createGetAccountsResponse = (command: GetAccountsCommand, result: TrezorGetAccountsResponse): GetAccountsResponsePayload => {
  return { id: command.id, command: command.command, payload: result, origin: command.origin }
}

addTrezorCommandHandler(TrezorCommand.Unlock, (command: UnlockCommand): Promise<UnlockResponsePayload> => {
  return new Promise(async (resolve) => {
    TrezorConnect.init({
      connectSrc: 'https://connect.trezor.io/8/',
      lazyLoad: false,
      manifest: {
        email: 'support@brave.com',
        appUrl: 'https://brave.com'
      }
    }).then(() => {
      resolve(createUnlockResponse(command, true))
    }).catch((error: any) => {
      resolve(createUnlockResponse(command, false, error))
    })
  })
})

addTrezorCommandHandler(TrezorCommand.GetAccounts, (command: GetAccountsCommand, source: Window): Promise<GetAccountsResponsePayload> => {
  return new Promise(async (resolve) => {
    TrezorConnect.getPublicKey({ bundle: command.paths }).then((result: TrezorGetAccountsResponse) => {
      resolve(createGetAccountsResponse(command, result))
    })
  })
})

addTrezorCommandHandler(TrezorCommand.SignTransaction, (command: SignTransactionCommand, source: Window): Promise<SignTransactionResponsePayload> => {
  return new Promise(async (resolve) => {
    TrezorConnect.ethereumSignTransaction(command.payload).then((result: Unsuccessful | Success<EthereumSignedTx>) => {
      resolve({ id: command.id, command: command.command, payload: result, origin: command.origin })
    })
  })
})

addTrezorCommandHandler(TrezorCommand.SignMessage, (command: SignMessageCommand, source: Window): Promise<SignMessageResponsePayload> => {
  return new Promise(async (resolve) => {
    TrezorConnect.ethereumSignMessage(command.payload).then((result: SignMessageResponse) => {
      resolve({ id: command.id, command: command.command, payload: result, origin: command.origin })
    })
  })
})

addTrezorCommandHandler(TrezorCommand.SignTypedMessage, (command: SignTypedMessageCommand, source: Window): Promise<SignTypedMessageResponsePayload> => {
  return new Promise(async (resolve) => {
    TrezorConnect.ethereumSignTypedData(command.payload).then((result: SignMessageResponse) => {
      resolve({ id: command.id, command: command.command, payload: result, origin: command.origin })
    })
  })
})
