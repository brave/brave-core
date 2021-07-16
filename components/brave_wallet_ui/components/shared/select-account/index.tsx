import * as React from 'react'
import SelectAccountItem from '../select-account-item'
import { UserAccountType } from '../../../constants/types'

export interface Props {
  accounts: UserAccountType[]
  onSelectAccount: (account: UserAccountType) => () => void
}

function SelectAccount (props: Props) {
  const { accounts, onSelectAccount } = props
  return <>{accounts.map((account) => <SelectAccountItem key={account.id} account={account} onSelectAccount={onSelectAccount(account)} />)}</>
}

export default SelectAccount
