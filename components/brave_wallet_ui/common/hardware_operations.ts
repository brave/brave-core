import { EthereumSignedTx } from 'trezor-connect/lib/typescript'
import { HardwareWalletAccount } from '../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import { HardwareWalletResponseCodeType } from '../constants/types'

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

export type GetAccountsHardwareOperationResult = HardwareOperationResult & {
  payload?: HardwareWalletAccount[]
}
