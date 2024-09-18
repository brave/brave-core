// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import TrezorConnect, { Success, Unsuccessful } from '@trezor/connect-web'
import { EthereumSignedTx } from '@trezor/connect/lib/types/api/ethereum'
import { PROTO } from '@trezor/connect/lib/constants'
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
  UnlockResponse,
  SignTypedMessageCommand,
  SignTypedMessageResponsePayload
} from '../common/hardware/trezor/trezor-messages'
import { addTrezorCommandHandler } from '../common/hardware/trezor/trezor-command-handler'

const hexPad = (hexString: string) => {
  if (hexString.length % 2 === 1) {
    return `0${hexString}`
  }
  return hexString
}

const createUnlockResponse = (
  command: UnlockCommand,
  result: boolean,
  error?: Unsuccessful
): UnlockResponsePayload => {
  const payload: UnlockResponse = !result && error ? error : { success: result }
  return {
    ...command,
    payload: payload
  }
}

const createGetAccountsResponse = (
  command: GetAccountsCommand,
  result: TrezorGetAccountsResponse
): GetAccountsResponsePayload => {
  return {
    ...command,
    payload: result
  }
}

addTrezorCommandHandler(
  TrezorCommand.Unlock,
  (command: UnlockCommand): Promise<UnlockResponsePayload> => {
    return new Promise(async (resolve) => {
      TrezorConnect.init({
        connectSrc: 'https://connect.trezor.io/9/',
        lazyLoad: false,
        manifest: {
          email: 'support@brave.com',
          appUrl: 'https://brave.com'
        }
      })
        .then(() => {
          resolve(createUnlockResponse(command, true))
        })
        .catch((error: any) => {
          resolve(createUnlockResponse(command, false, error))
        })
    })
  }
)

addTrezorCommandHandler(
  TrezorCommand.GetAccounts,
  (
    command: GetAccountsCommand,
    source: Window
  ): Promise<GetAccountsResponsePayload> => {
    return new Promise(async (resolve) => {
      TrezorConnect.getPublicKey({ bundle: command.paths }).then(
        (result: TrezorGetAccountsResponse) => {
          resolve(createGetAccountsResponse(command, result))
        }
      )
    })
  }
)

addTrezorCommandHandler(
  TrezorCommand.SignTransaction,
  (
    command: SignTransactionCommand,
    source: Window
  ): Promise<SignTransactionResponsePayload> => {
    return new Promise(async (resolve) => {
      TrezorConnect.ethereumSignTransaction(command.payload).then(
        (result: Unsuccessful | Success<EthereumSignedTx>) => {
          if (!result.success) {
            resolve({
              ...command,
              payload: {
                success: false,
                payload: {
                  error: result.payload.error,
                  code: result.payload.code
                }
              }
            })
            return
          }
          resolve({
            ...command,
            payload: {
              success: true,
              payload: {
                // https://docs.trezor.io/trezor-suite/packages/connect/methods/ethereumSignTransaction.html#result
                vBytes: Buffer.from(hexPad(result.payload.v.slice(2)), 'hex'),
                rBytes: Buffer.from(result.payload.r.slice(2), 'hex'),
                sBytes: Buffer.from(result.payload.s.slice(2), 'hex')
              }
            }
          })
        }
      )
    })
  }
)

addTrezorCommandHandler(
  TrezorCommand.SignMessage,
  (
    command: SignMessageCommand,
    source: Window
  ): Promise<SignMessageResponsePayload> => {
    return new Promise(async (resolve) => {
      TrezorConnect.ethereumSignMessage(command.payload).then(
        (result: Unsuccessful | Success<PROTO.MessageSignature>) => {
          if (!result.success) {
            resolve({
              ...command,
              payload: {
                success: false,
                payload: {
                  error: result.payload.error,
                  code: result.payload.code
                }
              }
            })
            return
          }

          resolve({
            ...command,
            // https://docs.trezor.io/trezor-suite/packages/connect/methods/ethereumSignMessage.html#result
            payload: {
              success: true,
              payload: {
                bytes: Buffer.from(result.payload.signature, 'hex')
              }
            }
          })
        }
      )
    })
  }
)

addTrezorCommandHandler(
  TrezorCommand.SignTypedMessage,
  (
    command: SignTypedMessageCommand,
    source: Window
  ): Promise<SignTypedMessageResponsePayload> => {
    return new Promise(async (resolve) => {
      TrezorConnect.ethereumSignTypedData(command.payload).then(
        (result: Unsuccessful | Success<PROTO.EthereumTypedDataSignature>) => {
          if (!result.success) {
            resolve({
              ...command,
              payload: {
                success: false,
                payload: {
                  error: result.payload.error,
                  code: result.payload.code
                }
              }
            })
            return
          }

          resolve({
            ...command,
            // https://docs.trezor.io/trezor-suite/packages/connect/methods/ethereumSignTypedData.html#result
            payload: {
              success: true,
              payload: {
                bytes: Buffer.from(result.payload.signature.slice(2), 'hex')
              }
            }
          })
        }
      )
    })
  }
)
