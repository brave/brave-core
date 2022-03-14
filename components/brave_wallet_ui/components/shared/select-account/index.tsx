import * as React from 'react'
import SelectAccountItem from '../select-account-item'
import { UserAccountType, BraveWallet, WalletAccountType } from '../../../constants/types'

export interface Props {
  accounts: WalletAccountType[]
  selectedAccount: UserAccountType
  onSelectAccount: (account: UserAccountType) => () => void
}

function SelectAccount (props: Props) {
  const { accounts, selectedAccount, onSelectAccount } = props

  // MULTICHAIN: Remove me once we support SOL and FIL transaction creation.
  // Will be implemented in these 2 issues
  // https://github.com/brave/brave-browser/issues/20698
  // https://github.com/brave/brave-browser/issues/20893
  const accountsList = React.useMemo(() => {
    return accounts.filter((account) =>
      account.coin !== BraveWallet.CoinType.SOL &&
      account.coin !== BraveWallet.CoinType.FIL)
  }, [accounts])

  return (
    <>
      {accountsList.map((account) =>
        <SelectAccountItem
          key={account.id}
          account={account}
          onSelectAccount={onSelectAccount(account)}
          selectedAccount={selectedAccount}
        />
      )}
    </>
  )
}

export default SelectAccount
