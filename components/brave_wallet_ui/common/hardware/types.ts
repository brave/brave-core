import { EthereumSignedTx } from './trezor/trezor-connect-types'
import { BraveWallet } from '../../constants/types'
import { SignedLotusMessage } from '@glif/filecoin-message'
import { LedgerError } from './ledgerjs/ledger-messages'

export const FilecoinNetworkTypes = [
  BraveWallet.FILECOIN_MAINNET, BraveWallet.FILECOIN_TESTNET
] as const
export type FilecoinNetwork = typeof FilecoinNetworkTypes[number]

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
  payload?: EthereumSignedTx | SignedLotusMessage | Buffer | string
}

export type GetAccountOperationResult = HardwareOperationResult & {
  payload?: Buffer
}

export interface TrezorBridgeAccountsPayload {
  success: boolean
  accounts: BraveWallet.HardwareWalletAccount[]
  error?: string
}

export enum LedgerDerivationPaths {
  LedgerLive = 'ledger-live',
  Legacy = 'legacy',
  Deprecated = 'deprecated'
}

export enum TrezorDerivationPaths {
  Default = 'trezor'
}

const DerivationSchemeTypes = [LedgerDerivationPaths.LedgerLive, LedgerDerivationPaths.Legacy, LedgerDerivationPaths.Deprecated, TrezorDerivationPaths.Default] as const
export type HardwareDerivationScheme = typeof DerivationSchemeTypes[number]

type HardwareWalletAccountBytesAddress = BraveWallet.HardwareWalletAccount & {
  addressBytes?: Buffer
}

export type GetAccountsHardwareOperationResult = HardwareOperationResult & {
  payload?: HardwareWalletAccountBytesAddress[]
}

// Did not create a string for these yet since it is
// likely these names will be returned from another service
// that will be localized.
export const FilecoinNetworkLocaleMapping = {
  [BraveWallet.FILECOIN_MAINNET]: 'Filecoin Mainnet',
  [BraveWallet.FILECOIN_TESTNET]: 'Filecoin Testnet'
}

// Batch size of accounts imported from the device in one step.
export const DerivationBatchSize = 4
