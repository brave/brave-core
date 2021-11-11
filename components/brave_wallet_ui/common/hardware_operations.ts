import { EthereumSignedTx } from 'trezor-connect/lib/typescript'

export interface SignHardwareTransactionType {
  success: boolean
  error?: string
}

export type HardwareOperationResult = {
  success: boolean
  error?: string
  code?: string | number
}

export type SignHardwareTransactionOperationResult = HardwareOperationResult & {
  payload?: EthereumSignedTx
}
