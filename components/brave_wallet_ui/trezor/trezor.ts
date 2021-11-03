import TrezorConnect, { Success, Unsuccessful } from 'trezor-connect'
import { HDNodeResponse } from 'trezor-connect/lib/typescript/trezor/protobuf'
import {
  TrezorCommand,
  UnlockCommand,
  UnlockResponse,
  GetAccountsCommand,
  TrezorGetAccountsResponse,
  GetAccountsResponsePayload,
  addTrezorCommandHandler
} from '../common/trezor/trezor-messages'

type TrezorGetPublicKeyResponse = Unsuccessful | Success<HDNodeResponse[]>
const createUnlockResponse = (command: UnlockCommand, result: Boolean, error?: Unsuccessful): UnlockResponse => {
  return { id: command.id, command: command.command, result: result, origin: command.origin, error: error }
}

const createGetAccountsResponse = (command: GetAccountsCommand, result: TrezorGetPublicKeyResponse): GetAccountsResponsePayload => {
  return { id: command.id, command: command.command, payload: result as TrezorGetAccountsResponse, origin: command.origin }
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
