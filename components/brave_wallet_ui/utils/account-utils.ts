import {
  AccountInfo,
  WalletAccountType
} from '../constants/types'

export const getAccountType = (info: AccountInfo): WalletAccountType['accountType'] => {
  if (info.hardware) {
    return info.hardware.vendor as 'Ledger' | 'Trezor'
  }
  return info.isImported ? 'Secondary' : 'Primary' as const
}

/** returns the new reference to an account obj within a new account list */
export const refreshSelectedAccount = (
  accounts: WalletAccountType[],
  selectedAccount: WalletAccountType
) => {
  return accounts.find(
    account => account.address === selectedAccount.address
  ) ?? selectedAccount
}
