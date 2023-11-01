// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import { SearchBar } from '../../shared/search-bar/index'
import { SelectAccount } from '../../shared/select-account/index'
import Header from '../../buy-send-swap/select-header'
import { getLocale } from '../../../../common/locale'
// Styled Components
import { SelectWrapper, SelectScrollSearchContainer } from '../shared-styles'

export interface Props {
  accounts: BraveWallet.AccountInfo[]
  selectedAccount?: BraveWallet.AccountInfo
  onSelectAccount: (account: BraveWallet.AccountInfo) => void
  onAddAccount?: () => void
  hasAddButton?: boolean
  onBack?: () => void
}

export function SelectAccountWithHeader(props: Props) {
  const {
    accounts,
    selectedAccount,
    onSelectAccount,
    onBack,
    onAddAccount,
    hasAddButton
  } = props
  const [filteredAccountList, setFilteredAccountList] =
    React.useState<BraveWallet.AccountInfo[]>(accounts)

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
        onClickAdd={onAddAccount}
        hasAddButton={hasAddButton}
      />
      <SearchBar
        placeholder={getLocale('braveWalletSearchAccount')}
        action={filterAccountList}
      />
      <SelectScrollSearchContainer>
        <SelectAccount
          accounts={filteredAccountList}
          selectedAccount={selectedAccount}
          onSelectAccount={onSelectAccount}
        />
      </SelectScrollSearchContainer>
    </SelectWrapper>
  )
}

export default SelectAccountWithHeader
