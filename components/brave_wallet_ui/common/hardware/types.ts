// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, FilecoinNetwork } from '../../constants/types'
import { LedgerError } from './ledgerjs/ledger-messages'
import { EthereumSignedTx } from './ledgerjs/eth-ledger-messages'
import { FilSignedLotusMessage } from './ledgerjs/fil-ledger-messages'
import { HardwareVendor } from '../api/hardware_keyrings'

export type HardwareWalletResponseCodeType =
  | 'deviceNotConnected'
  | 'deviceBusy'
  | 'openLedgerApp'
  | 'transactionRejected'
  | 'unauthorized'

export interface SignHardwareTransactionType {
  success: boolean
  error?: string
  deviceError?: HardwareWalletResponseCodeType
}
export interface SignatureVRS {
  v: number
  r: string
  s: string
}

export type HardwareOperationResult = {
  success: boolean
  error?: string | LedgerError
  code?: string | number
}

export type SignHardwareOperationResult = HardwareOperationResult & {
  payload?: EthereumSignedTx | FilSignedLotusMessage | Buffer | string
}

export enum LedgerDerivationPaths {
  LedgerLive = 'ledger-live',
  Legacy = 'legacy',
  Deprecated = 'deprecated'
}

export enum TrezorDerivationPaths {
  Default = 'trezor'
}

export enum SolDerivationPaths {
  Default = 'default',
  LedgerLive = 'ledger-live',
  Bip44Root = 'bip-44-root'
}

const DerivationSchemeTypes = [
  LedgerDerivationPaths.LedgerLive,
  LedgerDerivationPaths.Legacy,
  LedgerDerivationPaths.Deprecated,
  TrezorDerivationPaths.Default,
  SolDerivationPaths.Default,
  SolDerivationPaths.LedgerLive,
  SolDerivationPaths.Bip44Root
] as const
export type HardwareDerivationScheme = (typeof DerivationSchemeTypes)[number]

export type LedgerDerivationPath =
  (typeof LedgerDerivationPaths)[keyof typeof LedgerDerivationPaths]

export type TrezorDerivationPath =
  (typeof TrezorDerivationPaths)[keyof typeof TrezorDerivationPaths]

type HardwareWalletAccountBytesAddress = BraveWallet.HardwareWalletAccount & {
  addressBytes?: Buffer
}

export type GetAccountsHardwareOperationResult = HardwareOperationResult & {
  payload?: HardwareWalletAccountBytesAddress[]
}

// Batch size of accounts imported from the device in one step.
export const DerivationBatchSize = 4

export interface HardwareWalletConnectOpts {
  hardware: HardwareVendor
  // TODO: add currency and network as enums
  // currency: string
  // network: string

  startIndex: number
  stopIndex: number
  scheme?: HardwareDerivationScheme
  network?: FilecoinNetwork
  coin: BraveWallet.CoinType
  onAuthorized: () => void
}
