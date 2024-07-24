// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  HardwareWalletResponseCodeType
} from '../../constants/types'
import {
  EthereumSignedTx,
  FilSignedLotusMessage,
  LedgerError
} from './ledgerjs/ledger-messages'

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

export enum DerivationScheme {
  EthLedgerLive = 'EthLedgerLive',
  EthLedgerLegacy = 'EthLedgerLegacy',
  EthLedgerDeprecated = 'EthLedgerDeprecated',
  SolLedgerDefault = 'SolLedgerDefault',
  SolLedgerLedgerLive = 'SolLedgerLedgerLive',
  SolLedgerBip44Root = 'SolLedgerBip44Root',
  FilLedgerMainnet = 'FilLedgerMainnet',
  FilLedgerTestnet = 'FilLedgerTestnet',
  BtcLedgerMainnet = 'BtcLedgerMainnet',
  BtcLedgerTestnet = 'BtcLedgerTestnet',

  EthTrezorDefault = 'EthTrezorDefault'
}

export interface HardwareImportScheme {
  derivationScheme: DerivationScheme
  coin: BraveWallet.CoinType
  keyringId: BraveWallet.KeyringId
  vendor: BraveWallet.HardwareVendor
  name: string
  fixedNetwork?: string
  singleAccount?: boolean
  pathTemplate: (index: number | 'x') => string
}

export const EthLedgerLiveHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.EthLedgerLive,
  coin: BraveWallet.CoinType.ETH,
  keyringId: BraveWallet.KeyringId.kDefault,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: 'Ledger Live',
  pathTemplate: (index) => `m/44'/60'/${index}'/0/0`
}

export const EthLedgerLegacyHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.EthLedgerLegacy,
  coin: BraveWallet.CoinType.ETH,
  keyringId: BraveWallet.KeyringId.kDefault,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: 'Legacy (MEW/MyCrypto)',
  pathTemplate: (index) => `m/44'/60'/0'/${index}`
}

export const EthLedgerDeprecatedHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.EthLedgerDeprecated,
  coin: BraveWallet.CoinType.ETH,
  keyringId: BraveWallet.KeyringId.kDefault,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: 'Deprecated (Not recommended)',
  pathTemplate: (index) => `m/44'/60'/${index}'/0`
}

export const SolLedgerDefaultHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.SolLedgerDefault,
  coin: BraveWallet.CoinType.SOL,
  keyringId: BraveWallet.KeyringId.kSolana,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: 'Default',
  pathTemplate: (index) => `44'/501'/${index}'/0'`
}

export const SolLedgerLiveHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.SolLedgerLedgerLive,
  coin: BraveWallet.CoinType.SOL,
  keyringId: BraveWallet.KeyringId.kSolana,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: 'Ledger Live',
  pathTemplate: (index) => `44'/501'/${index}'`
}

export const SolLedgerBip44RootHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.SolLedgerBip44Root,
  coin: BraveWallet.CoinType.SOL,
  keyringId: BraveWallet.KeyringId.kSolana,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: 'Bip44 Root',
  singleAccount: true,
  pathTemplate: (index) => `44'/501'`
}

export const FilLedgerMainnetHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.FilLedgerMainnet,
  coin: BraveWallet.CoinType.FIL,
  keyringId: BraveWallet.KeyringId.kFilecoin,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: '',
  fixedNetwork: BraveWallet.FILECOIN_MAINNET,
  pathTemplate: (index) => `m/44'/461'/0'/0/${index}`
}

export const FilLedgerTestnetHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.FilLedgerTestnet,
  coin: BraveWallet.CoinType.FIL,
  keyringId: BraveWallet.KeyringId.kFilecoinTestnet,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: '',
  fixedNetwork: BraveWallet.FILECOIN_TESTNET,
  pathTemplate: (index) => `m/44'/1'/0'/0/${index}`
}

export const BtcLedgerMainnetHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.BtcLedgerMainnet,
  coin: BraveWallet.CoinType.BTC,
  keyringId: BraveWallet.KeyringId.kBitcoinImport,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: '',
  fixedNetwork: BraveWallet.BITCOIN_MAINNET,
  pathTemplate: (index) => `84'/0'/${index}'`
}

export const BtcLedgerTestnetHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.BtcLedgerTestnet,
  coin: BraveWallet.CoinType.BTC,
  keyringId: BraveWallet.KeyringId.kBitcoinImportTestnet,
  vendor: BraveWallet.HardwareVendor.kLedger,
  name: '',
  fixedNetwork: BraveWallet.BITCOIN_TESTNET,
  pathTemplate: (index) => `84'/1'/${index}'`
}

export const EthTrezorDefaultHardwareImportScheme: HardwareImportScheme = {
  derivationScheme: DerivationScheme.EthTrezorDefault,
  coin: BraveWallet.CoinType.ETH,
  keyringId: BraveWallet.KeyringId.kDefault,
  vendor: BraveWallet.HardwareVendor.kTrezor,
  name: 'Default',
  pathTemplate: (index) => `m/44'/60'/0'/0/${index}`
}

export const AllHardwareImportSchemes: HardwareImportScheme[] = [
  EthLedgerLiveHardwareImportScheme,
  EthLedgerLegacyHardwareImportScheme,
  EthLedgerDeprecatedHardwareImportScheme,

  SolLedgerDefaultHardwareImportScheme,
  SolLedgerLiveHardwareImportScheme,
  SolLedgerBip44RootHardwareImportScheme,

  FilLedgerMainnetHardwareImportScheme,
  FilLedgerTestnetHardwareImportScheme,

  BtcLedgerMainnetHardwareImportScheme,
  BtcLedgerTestnetHardwareImportScheme,

  EthTrezorDefaultHardwareImportScheme
]

export type GetAccountsHardwareOperationResult = HardwareOperationResult & {
  payload?: AccountFromDevice[]
}

// Batch size of accounts imported from the device in one step.
export const DerivationBatchSize = 5

export interface FetchHardwareWalletAccountsProps {
  scheme: HardwareImportScheme
  startIndex: number
  count: number
  onAuthorized: () => void
}

export interface AccountFromDevice {
  address: string
  derivationPath: string
  alreadyInWallet?: boolean
  shouldAddToWallet?: boolean
}
