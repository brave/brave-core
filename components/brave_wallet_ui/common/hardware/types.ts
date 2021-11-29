import { EthereumSignedTx } from 'trezor-connect/lib/typescript'
import { HardwareVendor } from '../api/getKeyringsByType'

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
  payload?: EthereumSignedTx
}

export type SignHardwareMessageOperationResult = HardwareOperationResult & {
  payload?: string
}

export interface TrezorBridgeAccountsPayload {
  success: boolean
  accounts: HardwareWalletAccount[]
  error?: string
}

// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export interface HardwareWalletAccount {
  address: string
  derivationPath: string
  name: string
  hardwareVendor: HardwareVendor
  deviceId: string
}

export enum LedgerDerivationPaths {
  LedgerLive = 'ledger-live',
  Legacy = 'legacy'
}

export enum TrezorDerivationPaths {
  Default = 'trezor'
}

const DerivationSchemeTypes = [LedgerDerivationPaths.LedgerLive, LedgerDerivationPaths.Legacy, TrezorDerivationPaths.Default] as const
export type HardwareDerivationScheme = typeof DerivationSchemeTypes[number]
