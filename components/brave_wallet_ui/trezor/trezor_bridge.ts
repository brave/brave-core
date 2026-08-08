/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import TrezorConnect, { DEVICE, DEVICE_EVENT } from '@trezor/connect-web'
import { EthereumSignedTx } from '@trezor/connect/lib/types/api/ethereum'
import { PROTO } from '@trezor/connect/lib/constants'
import { Unsuccessful, Success } from '../common/hardware/trezor/trezor-connect-types'
import * as TrezorMojom //
  from 'gen/brave/components/brave_wallet/common/trezor_bridge.mojom.m.js'

const stripUndefinedFields = <T extends object>(obj: T): T => {
  return Object.fromEntries(
    Object.entries(obj).filter(([_, v]) => v !== undefined),
  ) as T
}

let deviceName: string = ''

TrezorConnect.on(DEVICE_EVENT, (event) => {
  if (event.type === DEVICE.CONNECT || event.type === DEVICE.CHANGED) {
    deviceName = event.payload.name
  } else if (event.type === DEVICE.DISCONNECT) {
    deviceName = ''
  }
})

const toHex = (bytes: number[]): string => `0x${Buffer.from(bytes).toString('hex')}`

const hexPad = (hexString: string) => {
  if (hexString.length % 2 === 1) {
    return `0${hexString}`
  }
  return hexString
}

const toTrezorError = (unsuccess: Unsuccessful): TrezorMojom.TrezorError => {
  return {
    message: unsuccess.payload.error,
    code: unsuccess.payload.code ?? undefined,
  }
}

// Runs inside the untrusted `chrome-untrusted://trezor-bridge` frame (when the
// frame is running in mojo mode) and implements the `TrezorBridge` mojo
// interface. This is the Mojom equivalent of the postMessage
// `TrezorCommandHandler`; the actual device I/O (`@trezor/connect-web` calls,
// which themselves talk to Trezor's own `https://connect.trezor.io` origin in
// a nested iframe) is intentionally the same, only the outer transport
// differs.
export class TrezorBridge implements TrezorMojom.TrezorBridgeInterface {
  async unlock(): Promise<{ error: TrezorMojom.TrezorError | null }> {
    try {
      await TrezorConnect.init({
        lazyLoad: false,
        manifest: {
          appName: 'Brave Browser',
          email: 'support@brave.com',
          appUrl: 'https://brave.com',
        },
      })
      return { error: null }
    } catch (error: any) {
      return {
        error: {
          message: error?.payload?.error ?? '',
          code: error?.payload?.code ?? undefined,
        },
      }
    }
  }

  async getDeviceName(): Promise<{ result: TrezorMojom.TrezorDeviceNameResult }> {
    return { result: stripUndefinedFields({ deviceName, error: undefined }) }
  }

  async getAccounts(
    paths: string[],
  ): Promise<{ result: TrezorMojom.TrezorAccountsResult }> {
    const result = await TrezorConnect.getPublicKey({
      bundle: paths.map((path) => ({ path })),
    })
    if (!result.success) {
      return {
        result: stripUndefinedFields({
          accounts: undefined,
          error: toTrezorError(result),
        }),
      }
    }
    return {
      result: stripUndefinedFields({
        accounts: result.payload.map((account) => ({
          publicKey: account.publicKey,
          serializedPath: account.serializedPath,
          fingerprint: account.fingerprint,
        })),
        error: undefined,
      }),
    }
  }

  async signTransaction(
    path: string,
    transaction: TrezorMojom.TrezorEthTransaction,
  ): Promise<{ result: TrezorMojom.TrezorSignTransactionResult }> {
    const payload = transaction.legacy
      ? {
          path,
          transaction: {
            to: transaction.legacy.to,
            value: transaction.legacy.value,
            data: toHex(transaction.legacy.data),
            chainId: transaction.legacy.chainId,
            nonce: transaction.legacy.nonce,
            gasLimit: transaction.legacy.gasLimit,
            gasPrice: transaction.legacy.gasPrice,
          },
        }
      : {
          path,
          transaction: {
            to: transaction.eip1559!.to,
            value: transaction.eip1559!.value,
            data: toHex(transaction.eip1559!.data),
            chainId: transaction.eip1559!.chainId,
            nonce: transaction.eip1559!.nonce,
            gasLimit: transaction.eip1559!.gasLimit,
            maxFeePerGas: transaction.eip1559!.maxFeePerGas,
            maxPriorityFeePerGas: transaction.eip1559!.maxPriorityFeePerGas,
          },
        }
    const result: Unsuccessful | Success<EthereumSignedTx> =
      await TrezorConnect.ethereumSignTransaction(payload)
    if (!result.success) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toTrezorError(result),
        }),
      }
    }
    return {
      result: stripUndefinedFields({
        signature: {
          vBytes: [...Buffer.from(hexPad(result.payload.v.slice(2)), 'hex')],
          rBytes: [...Buffer.from(result.payload.r.slice(2), 'hex')],
          sBytes: [...Buffer.from(result.payload.s.slice(2), 'hex')],
        },
        error: undefined,
      }),
    }
  }

  async signMessage(
    path: string,
    message: string,
  ): Promise<{ result: TrezorMojom.TrezorSignMessageResult }> {
    const result: Unsuccessful | Success<PROTO.MessageSignature> =
      await TrezorConnect.ethereumSignMessage({ path, message })
    if (!result.success) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toTrezorError(result),
        }),
      }
    }
    return {
      result: stripUndefinedFields({
        signature: { bytes: [...Buffer.from(result.payload.signature, 'hex')] },
        error: undefined,
      }),
    }
  }

  async signTypedMessage(
    path: string,
    domainSeparatorHash: string,
    messageHash: string,
    typesJson: string,
    primaryType: string,
    domainJson: string,
    messageJson: string,
  ): Promise<{ result: TrezorMojom.TrezorSignMessageResult }> {
    const result: Unsuccessful | Success<PROTO.EthereumTypedDataSignature> =
      await TrezorConnect.ethereumSignTypedData({
        path,
        domain_separator_hash: domainSeparatorHash,
        message_hash: messageHash,
        metamask_v4_compat: true,
        data: {
          types: JSON.parse(typesJson),
          primaryType,
          domain: JSON.parse(domainJson),
          message: JSON.parse(messageJson),
        },
      })
    if (!result.success) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toTrezorError(result),
        }),
      }
    }
    return {
      result: stripUndefinedFields({
        signature: {
          bytes: [...Buffer.from(result.payload.signature.slice(2), 'hex')],
        },
        error: undefined,
      }),
    }
  }
}

// Bootstraps the mojo transport: creates the device-backed `TrezorBridge`
// implementation, binds it to a mojo pipe, and hands the remote up to the
// browser, which routes it to the embedding wallet page/panel renderer.
export const setupTrezorBridge = () => {
  const bridge = new TrezorBridge()
  const receiver = new TrezorMojom.TrezorBridgeReceiver(bridge)
  const uiHandler = TrezorMojom.TrezorBridgeUIHandler.getRemote()
  uiHandler.bindTrezorBridge(receiver.$.bindNewPipeAndPassRemote())
}
