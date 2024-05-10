// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import {
  reduceAccountDisplayName //
} from '../../../utils/reduce-account-name'

// Options
import {
  AllAccountsOption //
} from '../../../options/account-filter-options'

// Components
import {
  CreateAccountIcon //
} from '../create-account-icon/create-account-icon'

// Styled Components
import { DropdownFilter, DropdownOption } from './shared_dropdown.styles'
import { Row } from '../style'

interface Props {
  accounts: BraveWallet.AccountInfo[]
  selectedAccount: BraveWallet.AccountInfo
  showAllAccountsOption?: boolean
  onSelectAccount: (address: string) => void
  checkIsAccountOptionDisabled?: (account: BraveWallet.AccountInfo) => boolean
}

export const AccountsDropdown = (props: Props) => {
  const {
    accounts,
    selectedAccount,
    showAllAccountsOption,
    onSelectAccount,
    checkIsAccountOptionDisabled
  } = props

  return (
    <DropdownFilter
      onChange={(e) => onSelectAccount(e.value ?? '')}
      value={selectedAccount.accountId.address}
    >
      <Row
        slot='value'
        justifyContent='flex-start'
      >
        {selectedAccount.address !== AllAccountsOption.address && (
          <CreateAccountIcon
            size='tiny'
            account={selectedAccount}
            marginRight={8}
          />
        )}
        {selectedAccount.address === AllAccountsOption.address
          ? selectedAccount.name
          : reduceAccountDisplayName(selectedAccount.name, 12)}
      </Row>
      {showAllAccountsOption && (
        <leo-option value={AllAccountsOption.accountId.address}>
          <DropdownOption
            justifyContent='flex-start'
            isDisabled={false}
          >
            {AllAccountsOption.name}
          </DropdownOption>
        </leo-option>
      )}
      {accounts.map((account) => (
        <leo-option
          value={account.accountId.address}
          key={account.accountId.uniqueKey}
        >
          <DropdownOption
            justifyContent='flex-start'
            isDisabled={
              checkIsAccountOptionDisabled
                ? checkIsAccountOptionDisabled(account)
                : false
            }
          >
            {account.address !== AllAccountsOption.address && (
              <CreateAccountIcon
                size='tiny'
                account={account}
                marginRight={8}
              />
            )}
            {account.name}
          </DropdownOption>
        </leo-option>
      ))}
    </DropdownFilter>
  )
}
