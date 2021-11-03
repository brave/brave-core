// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import TrezorConnect, { Unsuccessful } from 'trezor-connect'

import {
  TrezorCommand,
  UnlockCommand,
  UnlockResponse,
  GetAccountsCommand,
  GetAccountsResponsePayload,
  TrezorGetPublicKeyResponse
} from '../common/trezor/trezor-messages'

import { addTrezorCommandHandler } from '../common/trezor/trezor-command-handler'

const createUnlockResponse = (command: UnlockCommand, result: Boolean, error?: Unsuccessful): UnlockResponse => {
  return { id: command.id, command: command.command, result: result, origin: command.origin, error: error }
}

const createGetAccountsResponse = (command: GetAccountsCommand, result: TrezorGetPublicKeyResponse): GetAccountsResponsePayload => {
  return { id: command.id, command: command.command, payload: result, origin: command.origin }
}

addTrezorCommandHandler(TrezorCommand.Unlock, (command: UnlockCommand): Promise<UnlockResponse> => {
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
    TrezorConnect.getPublicKey({ bundle: command.paths }).then((result: TrezorGetPublicKeyResponse) => {
      resolve(createGetAccountsResponse(command, result))
    })
  })
})
