import * as React from 'react'
import SelectAccountItem from '../select-account-item'
import { UserAccountType, WalletAccountType } from '../../../constants/types'

export interface Props {
  accounts: WalletAccountType[]
  selectedAccount: UserAccountType
  onSelectAccount: (account: UserAccountType) => () => void
}

function SelectAccount (props: Props) {
  const { accounts, selectedAccount, onSelectAccount } = props

  return (
    <>
      {accounts.map((account) =>
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
