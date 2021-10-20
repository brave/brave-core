import * as React from 'react'
import { UserAccountType } from '../../../constants/types'
import { SearchBar, SelectAccount } from '../../shared'
import Header from '../../buy-send-swap/select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  SelectWrapper,
  SelectScrollSearchContainer
} from '../../buy-send-swap/shared-styles'

export interface Props {
  accounts: UserAccountType[]
  onSelectAccount: (account: UserAccountType) => () => void
  onAddAccount?: () => void
  hasAddButton?: boolean
  onBack: () => void
}

function SelectAccountWithHeader (props: Props) {
  const { accounts, onSelectAccount, onBack, onAddAccount, hasAddButton } = props
  const [filteredAccountList, setFilteredAccountList] = React.useState<UserAccountType[]>(accounts)

  const filterAccountList = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setFilteredAccountList(accounts)
    } else {
      const filteredList = accounts.filter((item) => {
        return (
          item.name.toLowerCase() === search.toLowerCase() ||
          item.name.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setFilteredAccountList(filteredList)
    }
  }

  return (
    <SelectWrapper>
      <Header
        title={getLocale('braveWalletSelectAccount')}
        onBack={onBack}
        onAddAccount={onAddAccount}
        hasAddButton={hasAddButton}
      />
      <SearchBar placeholder={getLocale('braveWalletSearchAccount')} action={filterAccountList} />
      <SelectScrollSearchContainer>
        <SelectAccount
          accounts={filteredAccountList}
          onSelectAccount={onSelectAccount}
        />
      </SelectScrollSearchContainer>
    </SelectWrapper>
  )
}

export default SelectAccountWithHeader
