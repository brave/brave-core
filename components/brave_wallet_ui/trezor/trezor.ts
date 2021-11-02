import TrezorConnect, { Success, Unsuccessful } from 'trezor-connect'
import { HDNodeResponse } from 'trezor-connect/lib/typescript/trezor/protobuf'
import {
  TrezorCommand,
  kTrezorBridgeOwner,
  UnlockCommand,
  postResponseToWallet,
  UnlockResponse,
  GetAccountsCommand,
  TrezorGetAccountsResponse,
  GetAccountsResponsePayload
} from '../common/trezor/trezor-messages'

type TrezorGetPublicKeyResponse = Unsuccessful | Success<HDNodeResponse[]>
const createUnlockResponse = (command: UnlockCommand, result: Boolean, error?: Unsuccessful): UnlockResponse => {
  return { id: command.id, command: command.command, result: result, origin: command.origin, error: error }
}

const createGetAccountsResponse = (command: GetAccountsCommand, result: TrezorGetPublicKeyResponse): GetAccountsResponsePayload => {
  return { id: command.id, command: command.command, payload: result as TrezorGetAccountsResponse, origin: command.origin }
}

const unlock = async (command: UnlockCommand, source: Window) => {
  TrezorConnect.init({
    connectSrc: 'https://connect.trezor.io/8/',
    lazyLoad: false,
    manifest: {
      email: 'support@brave.com',
      appUrl: 'https://brave.com'
    }
  }).then(() => {
    postResponseToWallet(source, createUnlockResponse(command, true))
  }).catch((error) => {
    postResponseToWallet(source, createUnlockResponse(command, false, error))
  })
}

const getAccounts = async (command: GetAccountsCommand, source: Window) => {
  TrezorConnect.getPublicKey({ bundle: command.paths }).then((result: TrezorGetPublicKeyResponse) => {
    postResponseToWallet(source, createGetAccountsResponse(command, result))
  }).catch((error) => {
    postResponseToWallet(source, createGetAccountsResponse(command, error))
  })
}

window.addEventListener('message', (event: any/* MessageEvent<TrezorFrameCommand>*/) => {
  if (event.origin !== event.data.origin || event.type !== 'message' ||
      event.origin !== kTrezorBridgeOwner || !event.source) {
    return
  }
  if (event.data.command === TrezorCommand.Unlock) {
    return unlock(event.data, event.source as Window)
  }
  if (event.data.command === TrezorCommand.GetAccounts) {
    return getAccounts(event.data, event.source as Window)
  }
  return
})
