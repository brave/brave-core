/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as bs58 from 'bs58'
import {
  AccountFromDevice,
  DerivationSchemes,
  HardwareImportScheme,
  HardwareOperationError,
  HardwareOperationResult,
  HardwareOperationResultAccounts,
  HardwareOperationResultBitcoinSignature,
  HardwareOperationResultDeviceName,
  HardwareOperationResultEthereumSignatureBytes,
  HardwareOperationResultEthereumSignatureVRS,
  HardwareOperationResultFilecoinSignature,
  HardwareOperationResultSolanaSignature,
} from '../types'
import * as LedgerMojom from 'gen/brave/components/brave_wallet/common/ledger_bridge.mojom.m.js' //
import { ledgerBridgeRegistry } from './ledger_bridge_registry'
import EthereumLedgerBridgeKeyring from './eth_ledger_bridge_keyring'
import SolanaLedgerBridgeKeyring from './sol_ledger_bridge_keyring'
import FilecoinLedgerBridgeKeyring from './fil_ledger_bridge_keyring'
import BitcoinLedgerBridgeKeyring from './btc_ledger_bridge_keyring'

// The mojo keyrings extend the postMessage keyrings and override only the
// transport-facing methods, so that `instanceof` checks elsewhere (see
// common/async/hardware.ts) keep working and no other code needs to know which
// transport is in use. The inherited postMessage plumbing (createBridge /
// sendCommand) is never exercised because every public method is overridden.

function getBridge(): LedgerMojom.LedgerBridgeRemote {
  return ledgerBridgeRegistry.getBridge()
}

function toHardwareError(
  error: LedgerMojom.LedgerError,
): HardwareOperationError {
  return { success: false, error: error.message, code: error.code ?? undefined }
}

async function mojoUnlock(): Promise<HardwareOperationResult> {
  const bridge = getBridge()
  const { error } = await bridge.unlock()
  return error ? toHardwareError(error) : { success: true }
}

async function mojoGetDeviceName(): Promise<HardwareOperationResultDeviceName> {
  const bridge = getBridge()
  const { result } = await bridge.getDeviceName()
  if (result.error) {
    return toHardwareError(result.error)
  }
  return { success: true, deviceName: result.deviceName ?? '' }
}

async function finishAccounts(
  accounts: AccountFromDevice[],
): Promise<HardwareOperationResultAccounts> {
  const deviceName = await mojoGetDeviceName()
  return {
    success: true,
    accounts,
    deviceName: deviceName.success ? deviceName.deviceName : '',
  }
}

export class EthereumLedgerMojoBridgeKeyring extends EthereumLedgerBridgeKeyring {
  unlock = mojoUnlock
  getDeviceName = mojoGetDeviceName

  getAccounts = async (
    from: number,
    count: number,
    scheme: HardwareImportScheme,
  ): Promise<HardwareOperationResultAccounts> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const accounts: AccountFromDevice[] = []
    for (let i = 0; i < count; i++) {
      const path = scheme.pathTemplate(from + i)
      const { result } = await bridge.ethGetAccount(path)
      if (result.error) {
        return toHardwareError(result.error)
      }
      accounts.push({ address: result.account!.address, derivationPath: path })
    }
    return finishAccounts(accounts)
  }

  signTransaction = async (
    path: string,
    rawTxHex: string,
  ): Promise<HardwareOperationResultEthereumSignatureVRS> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.ethSignTransaction(path, rawTxHex)
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }

  signPersonalMessage = async (
    path: string,
    message: string,
  ): Promise<HardwareOperationResultEthereumSignatureBytes> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const messageHex = Buffer.from(message).toString('hex')
    const bridge = getBridge()
    const { result } = await bridge.ethSignPersonalMessage(path, messageHex)
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }

  signEip712Message = async (
    path: string,
    domainSeparatorHex: string,
    hashStructMessageHex: string,
  ): Promise<HardwareOperationResultEthereumSignatureBytes> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.ethSignEip712Message(
      path,
      domainSeparatorHex,
      hashStructMessageHex,
    )
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }
}

export class SolanaLedgerMojoBridgeKeyring extends SolanaLedgerBridgeKeyring {
  unlock = mojoUnlock
  getDeviceName = mojoGetDeviceName

  getAccounts = async (
    from: number,
    count: number,
    scheme: HardwareImportScheme,
  ): Promise<HardwareOperationResultAccounts> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    // The root path does not support an index.
    const paths =
      scheme.derivationScheme === DerivationSchemes.SolLedgerBip44Root
        ? [scheme.pathTemplate(0)]
        : Array.from({ length: count }, (_, i) => scheme.pathTemplate(from + i))

    const bridge = getBridge()
    const accounts: AccountFromDevice[] = []
    for (const path of paths) {
      const { result } = await bridge.solGetAccount(path)
      if (result.error) {
        return toHardwareError(result.error)
      }
      accounts.push({
        address: bs58.encode(Buffer.from(result.account!.address)),
        derivationPath: path,
      })
    }
    return finishAccounts(accounts)
  }

  signTransaction = async (
    path: string,
    rawTxBytes: Buffer,
  ): Promise<HardwareOperationResultSolanaSignature> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.solSignTransaction(path, [...rawTxBytes])
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }
}

export class FilecoinLedgerMojoBridgeKeyring extends FilecoinLedgerBridgeKeyring {
  unlock = mojoUnlock
  getDeviceName = mojoGetDeviceName

  getAccounts = async (
    from: number,
    count: number,
    scheme: HardwareImportScheme,
  ): Promise<HardwareOperationResultAccounts> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const isTestnet =
      scheme.derivationScheme === DerivationSchemes.FilLedgerTestnet
    const bridge = getBridge()
    const { result } = await bridge.filGetAccount(from, count, isTestnet)
    if (result.error) {
      return toHardwareError(result.error)
    }
    const accounts: AccountFromDevice[] = result.accounts!.accounts.map(
      (address, i) => ({
        address,
        derivationPath: scheme.pathTemplate(from + i),
      }),
    )
    return finishAccounts(accounts)
  }

  signTransaction = async (
    message: string,
  ): Promise<HardwareOperationResultFilecoinSignature> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.filSignTransaction(message)
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }
}

export class BitcoinLedgerMojoBridgeKeyring extends BitcoinLedgerBridgeKeyring {
  unlock = mojoUnlock
  getDeviceName = mojoGetDeviceName

  getAccounts = async (
    from: number,
    count: number,
    scheme: HardwareImportScheme,
  ): Promise<HardwareOperationResultAccounts> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const xpubVersion =
      scheme.derivationScheme === DerivationSchemes.BtcLedgerMainnet
        ? 0x0488b21e // xpub
        : 0x043587cf // tpub
    const bridge = getBridge()
    const accounts: AccountFromDevice[] = []
    for (let i = 0; i < count; i++) {
      const path = scheme.pathTemplate(from + i)
      const { result } = await bridge.btcGetAccount(path, xpubVersion)
      if (result.error) {
        return toHardwareError(result.error)
      }
      accounts.push({ address: result.account!.xpub, derivationPath: path })
    }
    return finishAccounts(accounts)
  }

  signTransaction = async (
    inputTransactions: Array<{
      txBytes: Buffer
      outputIndex: number
      associatedPath: string
    }>,
    outputScript: Buffer,
    changePath: string | undefined,
    lockTime: number,
  ): Promise<HardwareOperationResultBitcoinSignature> => {
    const unlockResult = await this.unlock()
    if (!unlockResult.success) {
      return unlockResult
    }
    const bridge = getBridge()
    const { result } = await bridge.btcSignTransaction(
      inputTransactions.map((i) => ({
        txBytes: [...i.txBytes],
        outputIndex: i.outputIndex,
        associatedPath: i.associatedPath,
      })),
      [...outputScript],
      changePath ?? null,
      lockTime,
    )
    if (result.error) {
      return toHardwareError(result.error)
    }
    return { success: true, signature: result.signature! }
  }
}
