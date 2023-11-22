// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Hooks
import {
  useOnClickOutside //
} from '../../../../../../common/hooks/useOnClickOutside'
import {
  useAccountsQuery //
} from '../../../../../../common/slices/api.slice.extra'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import {
  AccountListButton //
} from '../../buttons/account-list-button/account-list-button'

// Styled Components
import {
  SelectButton,
  SelectorBox,
  SelectorWrapper,
  StyledCaratDownIcon
} from './account-selector.style'
import { Text, HorizontalSpacer } from '../../shared-swap.styles'

interface Props {
  disabled?: boolean
  selectedAccount: BraveWallet.AccountInfo | undefined
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  showAccountSelector: boolean
  setShowAccountSelector: (value: boolean) => void
  onSelectAccount: (account: BraveWallet.AccountInfo) => void
}

export const AccountSelector = (props: Props) => {
  const {
    disabled,
    onSelectAccount,
    selectedAccount,
    selectedNetwork,
    showAccountSelector,
    setShowAccountSelector
  } = props

  // queries
  const { accounts } = useAccountsQuery()

  // Refs
  const accountSelectorRef = React.useRef<HTMLDivElement>(null)

  // Memos
  const networkAccounts = React.useMemo(() => {
    return accounts.filter(
      (account) => account.accountId.coin === selectedNetwork?.coin
    )
  }, [accounts, selectedNetwork])

  // Methods
  const onToggleShowAccountSelector = React.useCallback(() => {
    setShowAccountSelector(!showAccountSelector)
  }, [showAccountSelector, setShowAccountSelector])

  const onClickSelectAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      onSelectAccount(account)
      setShowAccountSelector(false)
    },
    [onSelectAccount, setShowAccountSelector]
  )

  // Hooks
  useOnClickOutside(
    accountSelectorRef,
    () => setShowAccountSelector(false),
    showAccountSelector
  )

  return (
    <SelectorWrapper ref={accountSelectorRef}>
      <SelectButton
        onClick={onToggleShowAccountSelector}
        disabled={disabled}
      >
        <Text
          textSize='12px'
          textColor='text02'
        >
          {selectedAccount
            ? selectedAccount.name
            : getLocale('braveSwapSelectAccount')}
        </Text>
        <HorizontalSpacer size={8} />
        <StyledCaratDownIcon
          size={20}
          name='carat-down'
        />
      </SelectButton>
      {showAccountSelector && (
        <SelectorBox>
          {networkAccounts.map((account) => (
            <AccountListButton
              account={account}
              onClick={onClickSelectAccount}
              key={account.accountId.uniqueKey}
            />
          ))}
        </SelectorBox>
      )}
    </SelectorWrapper>
  )
}
