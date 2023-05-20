// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import {
  WalletSelectors
} from '../../../../../../common/selectors'
import {
  useUnsafeWalletSelector
} from '../../../../../../common/hooks/use-safe-selector'

import {
  useGetSelectedChainQuery
} from '../../../../../../common/slices/api.slice'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'

// Hooks
import {
  useOnClickOutside
} from '../../../../../../common/hooks/useOnClickOutside'

// Types
import {
  WalletAccountType
} from '../../../../../../constants/types'

// Assets
// FIXME(douglashdaniel): This is not the correct icon
import CaratDownIcon from '../../../assets/lp-icons/0x.svg'

// Components
import { AccountListButton } from '../../buttons/account-list-button/account-list-button'

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
  selectedAccount: WalletAccountType | undefined
  showAccountSelector: boolean
  setShowAccountSelector: (value: boolean) => void
  onSelectAccount: (account: WalletAccountType) => void
}

export const AccountSelector = (props: Props) => {
  const {
    disabled,
    onSelectAccount,
    selectedAccount,
    showAccountSelector,
    setShowAccountSelector
  } = props

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // Selectors
  const accounts: WalletAccountType[] = useUnsafeWalletSelector(WalletSelectors.accounts)

  // Refs
  const accountSelectorRef = React.useRef<HTMLDivElement>(null)

  // Memos
  const networkAccounts = React.useMemo(() => {
    return accounts.filter(account => account.coin === selectedNetwork?.coin)
  }, [accounts, selectedNetwork])

  // Methods
  const onToggleShowAccountSelector = React.useCallback(() => {
    setShowAccountSelector(!showAccountSelector)
  }, [showAccountSelector, setShowAccountSelector])

  const onClickSelectAccount = React.useCallback(
    (account: WalletAccountType) => {
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
      <SelectButton onClick={onToggleShowAccountSelector} disabled={disabled}>
        <Text textSize='12px' textColor='text02'>
          {selectedAccount ? selectedAccount.name : getLocale('braveSwapSelectAccount')}
        </Text>
        <HorizontalSpacer size={8} />
        <StyledCaratDownIcon size={16} icon={CaratDownIcon} />
      </SelectButton>
      {showAccountSelector && (
        <SelectorBox>
          {networkAccounts.map(account => (
            <AccountListButton account={account} onClick={onClickSelectAccount} key={account.address} />
          ))}
        </SelectorBox>
      )}
    </SelectorWrapper>
  )
}
