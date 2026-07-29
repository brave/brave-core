/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Transport from '@ledgerhq/hw-transport'
import TransportWebHID from '@ledgerhq/hw-transport-webhid'
import { TransportStatusError } from '@ledgerhq/errors'
import Eth from '@ledgerhq/hw-app-eth'
import Sol from '@ledgerhq/hw-app-solana'
import Btc from '@ledgerhq/hw-app-btc'
import { BufferReader, BufferWriter } from '@ledgerhq/hw-app-btc/buffertools'
import { CreateTransactionArg } from '@ledgerhq/hw-app-btc/lib/createTransaction'
import { CoinType } from '@glif/filecoin-address'
import { LotusMessage, SignedLotusMessage } from '@glif/filecoin-message'
import {
  LedgerProvider,
  TransportWrapper,
} from '@glif/filecoin-wallet-provider'
import * as LedgerMojom from 'gen/brave/components/brave_wallet/common/ledger_bridge.mojom.m.js' //

const stripUndefinedFields = <T extends object>(obj: T): T => {
  return Object.fromEntries(
    Object.entries(obj).filter(([_, v]) => v !== undefined),
  ) as T
}

export class LedgerBridge implements LedgerMojom.LedgerBridgeInterface {
  private deviceName: string = ''

  // Filecoin uses a persistent provider/transport (see @glif).
  private filTransportWrapper?: TransportWrapper
  private filProvider?: LedgerProvider

  // Verifies that a Ledger device is connected before creating a transport.
  private createTransport = async (): Promise<Transport> => {
    const devices = await TransportWebHID.list()
    if (devices.length === 0) {
      throw new Error('No Ledger device found.')
    }
    return TransportWebHID.create()
  }

  private fillDeviceName = async (): Promise<void> => {
    try {
      const transport = await this.createTransport()
      this.deviceName = transport.deviceModel?.productName ?? ''
      await transport.close()
    } catch (error) {}
  }

  async unlock(): Promise<{ error: LedgerMojom.LedgerError | null }> {
    await this.fillDeviceName()
    return { error: null }
  }

  async getDeviceName(): Promise<{ result: LedgerMojom.DeviceNameResult }> {
    if (!this.deviceName) {
      await this.fillDeviceName()
    }
    return {
      result: stripUndefinedFields({
        deviceName: this.deviceName,
        error: undefined,
      }),
    }
  }

  async ethGetAccount(
    path: string,
  ): Promise<{ result: LedgerMojom.EthAccountResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Eth(transport)
      const result = await app.getAddress(path)
      return {
        result: stripUndefinedFields({
          account: {
            publicKey: result.publicKey,
            address: result.address,
            chainCode: result.chainCode ?? undefined,
          },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          account: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  async ethSignTransaction(
    path: string,
    rawTxHex: string,
  ): Promise<{ result: LedgerMojom.EthVrsResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Eth(transport)
      // https://github.com/LedgerHQ/ledger-live/tree/develop/libs/ledgerjs/packages/hw-app-eth#examples-2
      const result = await app.signTransaction(path, rawTxHex)
      return {
        result: stripUndefinedFields({
          signature: {
            vBytes: hexToBytes(result.v),
            rBytes: hexToBytes(result.r),
            sBytes: hexToBytes(result.s),
          },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  async ethSignPersonalMessage(
    path: string,
    messageHex: string,
  ): Promise<{ result: LedgerMojom.EthBytesResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Eth(transport)
      // https://github.com/LedgerHQ/ledger-live/tree/develop/libs/ledgerjs/packages/hw-app-eth#examples-4
      const result = await app.signPersonalMessage(path, messageHex)
      return {
        result: stripUndefinedFields({
          signature: {
            bytes: concatVrsBytes(vToHex(result.v), result.r, result.s),
          },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  async ethSignEip712Message(
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string,
  ): Promise<{ result: LedgerMojom.EthBytesResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Eth(transport)
      // https://github.com/LedgerHQ/ledger-live/tree/develop/libs/ledgerjs/packages/hw-app-eth#examples-5
      const result = await app.signEIP712HashedMessage(
        path,
        domainSeparatorHex,
        hashStructMessageHex,
      )
      return {
        result: stripUndefinedFields({
          signature: {
            bytes: concatVrsBytes(vToHex(result.v), result.r, result.s),
          },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  async solGetAccount(
    path: string,
  ): Promise<{ result: LedgerMojom.SolAccountResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Sol(transport)
      const result = await app.getAddress(path)
      return {
        result: stripUndefinedFields({
          account: { address: [...result.address] },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          account: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  async solSignTransaction(
    path: string,
    rawTxBytes: number[],
  ): Promise<{ result: LedgerMojom.SolSignatureResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Sol(transport)
      const result = await app.signTransaction(path, Buffer.from(rawTxBytes))
      return {
        result: stripUndefinedFields({
          signature: { bytes: [...result.signature] },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  async filGetAccount(
    from: number,
    count: number,
    isTestnet: boolean,
  ): Promise<{ result: LedgerMojom.FilAccountResult }> {
    try {
      if (!this.filProvider && !(await this.makeFilProvider())) {
        return {
          result: stripUndefinedFields({
            accounts: undefined,
            error: { message: '', code: undefined },
          }),
        }
      }
      const accounts = await this.filProvider!.getAccounts(
        from,
        from + count,
        isTestnet ? CoinType.TEST : CoinType.MAIN,
      )
      return {
        result: stripUndefinedFields({
          accounts: { accounts },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          accounts: undefined,
          error: toLedgerError(error),
        }),
      }
    }
  }

  async filSignTransaction(
    message: string,
  ): Promise<{ result: LedgerMojom.FilSignatureResult }> {
    try {
      if (!this.filProvider && !(await this.makeFilProvider())) {
        return {
          result: stripUndefinedFields({
            signature: undefined,
            error: { message: '', code: undefined },
          }),
        }
      }
      // Accounts should be warmed up before signing.
      await this.filProvider!.getAccounts()
      const parsed = JSON.parse(message)
      const lotusMessage: LotusMessage = {
        To: parsed.To,
        From: parsed.From,
        Nonce: parsed.Nonce,
        Value: parsed.Value,
        GasPremium: parsed.GasPremium,
        GasLimit: parsed.GasLimit,
        GasFeeCap: parsed.GasFeeCap,
        Method: parsed.Method,
        Params: parsed.Params,
      }
      const signed: SignedLotusMessage = await this.filProvider!.sign(
        parsed.From,
        lotusMessage,
      )
      return {
        result: stripUndefinedFields({
          signature: { signedMessageJson: JSON.stringify(signed) },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toLedgerError(error),
        }),
      }
    }
  }

  async btcGetAccount(
    path: string,
    xpubVersion: number,
  ): Promise<{ result: LedgerMojom.BtcAccountResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Btc({ transport })
      const result = await app.getWalletXpub({ path, xpubVersion })
      return {
        result: stripUndefinedFields({
          account: { xpub: result },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          account: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  async btcSignTransaction(
    inputTransactions: LedgerMojom.BtcInputTransaction[],
    outputScript: number[],
    changePath: string | null,
    lockTime: number,
  ): Promise<{ result: LedgerMojom.BtcSignatureResult }> {
    let transport: Transport | undefined
    try {
      transport = await this.createTransport()
      const app = new Btc({ transport })
      const signedTransactionHex = await app.createPaymentTransaction({
        inputs: inputTransactions.map((i) => {
          return [
            app.splitTransaction(Buffer.from(i.txBytes).toString('hex'), true),
            i.outputIndex,
            undefined,
            0xfffffffd, // sequence number same as for core transactions.
          ]
        }),
        associatedKeysets: inputTransactions.map((i) => i.associatedPath),
        outputScriptHex: Buffer.from(outputScript).toString('hex'),
        additionals: ['bech32'],
        changePath: changePath ?? undefined,
        lockTime,
        sigHashType: 1, // SIGHASH_ALL
        segwit: true,
      } satisfies CreateTransactionArg)

      const signedTransaction = app.splitTransaction(signedTransactionHex, true)
      if (!signedTransaction.witness) {
        throw new Error('Unexpected empty witness.')
      }

      const witnessArray: number[][] = []
      const witnessesReader = new BufferReader(signedTransaction.witness)
      while (!witnessesReader.available) {
        const witnessField = witnessesReader.readVector()
        if (witnessField.length !== 2) {
          throw new Error('Invalid witness field size.')
        }
        const witnessBuf = new BufferWriter()
        witnessBuf.writeVarInt(2)
        witnessBuf.writeVarSlice(witnessField[0])
        witnessBuf.writeVarSlice(witnessField[1])
        witnessArray.push([...witnessBuf.buffer()])
      }

      if (witnessArray.length !== inputTransactions.length) {
        throw new Error('Invalid number of witness fields.')
      }

      return {
        result: stripUndefinedFields({
          signature: { witnessArray },
          error: undefined,
        }),
      }
    } catch (error) {
      return {
        result: stripUndefinedFields({
          signature: undefined,
          error: toLedgerError(error),
        }),
      }
    } finally {
      await transport?.close()
    }
  }

  private makeFilProvider = async (): Promise<boolean> => {
    if (this.filTransportWrapper) {
      await this.filTransportWrapper.disconnect()
    }
    this.filTransportWrapper = new TransportWrapper()
    try {
      await this.filTransportWrapper.connect()
      this.filTransportWrapper.transport.on(
        'disconnect',
        this.onFilDisconnected,
      )

      const filProvider = new LedgerProvider({
        transport: this.filTransportWrapper.transport,
        minLedgerVersion: { major: 0, minor: 0, patch: 1 },
      })
      if (!(await filProvider.ready())) {
        return false
      }
      this.filProvider = filProvider
      return true
    } catch (e) {
      return false
    }
  }

  private onFilDisconnected = (e: any) => {
    if (e.name !== 'DisconnectedDevice') {
      return
    }
    this.filProvider = undefined
    this.filTransportWrapper = undefined
  }
}

function toLedgerError(error: unknown): LedgerMojom.LedgerError {
  if ((error as Error).message === 'The device is already open.') {
    return {
      message: (error as Error).message,
      code: '1', // LedgerBridgeErrorCodes.CommandInProgress,
    }
  }
  return {
    message: (error as Error).message,
    code:
      error instanceof TransportStatusError
        ? String(error.statusCode)
        : undefined,
  }
}

function hexToBytes(hex: string): number[] {
  return [...Buffer.from(hex, 'hex')]
}

// Concatenates r|s|v as a single byte array (personal / EIP-712 signatures).
function concatVrsBytes(v: string, r: string, s: string): number[] {
  return [
    ...Buffer.concat([
      Buffer.from(r, 'hex'),
      Buffer.from(s, 'hex'),
      Buffer.from(v, 'hex'),
    ]),
  ]
}

function vToHex(vNumber: number): string {
  let v = vNumber < 27 ? vNumber.toString(16) : (vNumber - 27).toString(16)
  // Pad with a leading zero if under 0x10 (single hex char).
  if (v.length < 2) {
    v = `0${v}`
  }
  return v
}

// Bootstraps the mojo transport: creates the device-backed `LedgerBridge`
// implementation, binds it to a mojo pipe, and hands the remote up to the
// browser, which routes it to the embedding wallet page/panel renderer.
export const setupLedgerBridge = () => {
  const bridge = new LedgerBridge()
  const receiver = new LedgerMojom.LedgerBridgeReceiver(bridge)
  const uiHandler = LedgerMojom.LedgerBridgeUIHandler.getRemote()
  uiHandler.bindLedgerBridge(receiver.$.bindNewPipeAndPassRemote())
}
