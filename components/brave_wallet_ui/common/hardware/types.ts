import { EthereumSignedTx } from 'trezor-connect/lib/typescript'
import { BraveWallet } from '../../constants/types'
import { SignedLotusMessage } from '@glif/filecoin-message'

export const FilecoinNetworkTypes = [
  BraveWallet.FILECOIN_MAINNET, BraveWallet.FILECOIN_TESTNET
] as const
export type FilecoinNetwork = typeof FilecoinNetworkTypes[number]

export type HardwareWalletResponseCodeType =
  | 'deviceNotConnected'
  | 'deviceBusy'
  | 'openEthereumApp'
  | 'transactionRejected'

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
  error?: string
  code?: string | number
}

export type SignHardwareTransactionOperationResult = HardwareOperationResult & {
  payload?: EthereumSignedTx | SignedLotusMessage
}

export type SignHardwareMessageOperationResult = HardwareOperationResult & {
  payload?: string
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

export type GetAccountsHardwareOperationResult = HardwareOperationResult & {
  payload?: BraveWallet.HardwareWalletAccount[]
}

// Batch size of accounts imported from the device in one step.
export const DerivationBatchSize = 4
