// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Redux
import { useSelector } from 'react-redux'

// Types
import { BraveWallet, WalletState } from '../../../constants/types'

// Options
import {
  AllAccountsOption,
  isAllAccountsOptionFilter
} from '../../../options/account-filter-options'
import { AllNetworksOption } from '../../../options/network-filter-options'

// Components
import { AccountFilterItem } from './account-filter-item'

// Styles
import { AccountCircle } from './account-filter-selector.style'

import {
  StyledWrapper,
  DropDown,
  DropDownButton,
  DropDownIcon,
  SelectorLeftSide,
  ClickAwayArea
} from '../network-filter-selector/style'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'
import { useAccountsQuery } from '../../../common/slices/api.slice.extra'

interface Props {
  onSelectAccount: (
    account: Pick<BraveWallet.AccountInfo, 'accountId' | 'address' | 'name'>
  ) => void
  selectedAccount?: Pick<
    BraveWallet.AccountInfo,
    'accountId' | 'address' | 'name'
  >
  selectedNetwork?: BraveWallet.NetworkInfo
}

export const AccountFilterSelector = ({
  onSelectAccount,
  selectedAccount: accountProp,
  selectedNetwork: networkProp
}: Props) => {
  // Wallet State
  const selectedAccountFilter = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.selectedAccountFilter
  )
  const selectedNetworkFilter = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter
  )

  // Queries
  const { accounts } = useAccountsQuery()

  // State
  const [isOpen, setIsOpen] = React.useState(false)

  // Methods
  const onClick = React.useCallback(() => {
    setIsOpen((prevIsOpen) => !prevIsOpen)
  }, [])

  const onSelectAccountAndClose = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      setIsOpen(false)
      onSelectAccount(account)
    },
    [onSelectAccount]
  )

  // Memos
  const selectedAccount =
    accountProp ||
    [...accounts, AllAccountsOption].find(
      (account) => account.accountId.uniqueKey === selectedAccountFilter
    ) ||
    AllAccountsOption
  const selectedNetwork = networkProp || selectedNetworkFilter

  const orb = useAccountOrb(selectedAccount)

  // Filters accounts by network if a selectedNetworkFilter is selected
  const accountsFilteredBySelectedNetworkFilter = React.useMemo(() => {
    return selectedNetwork.chainId === AllNetworksOption.chainId
      ? accounts
      : accounts.filter(
          (account) => account.accountId.coin === selectedNetwork.coin
        )
  }, [accounts, selectedNetwork])

  const accountsList: BraveWallet.AccountInfo[] = React.useMemo(() => {
    return [AllAccountsOption, ...accountsFilteredBySelectedNetworkFilter]
  }, [accountsFilteredBySelectedNetworkFilter])

  return (
    <StyledWrapper>
      <DropDownButton onClick={onClick}>
        <SelectorLeftSide>
          {!isAllAccountsOptionFilter(selectedAccount.accountId.uniqueKey) && (
            <AccountCircle orb={orb} />
          )}
          {selectedAccount.name}
        </SelectorLeftSide>
        <DropDownIcon />
      </DropDownButton>
      {isOpen && (
        <DropDown>
          {accountsList.map((account) => (
            <AccountFilterItem
              key={account.accountId.uniqueKey}
              account={account}
              selected={
                account.accountId.uniqueKey ===
                selectedAccount.accountId.uniqueKey
              }
              showCircle={
                !isAllAccountsOptionFilter(account.accountId.uniqueKey)
              }
              onSelectAccount={onSelectAccountAndClose}
            />
          ))}
        </DropDown>
      )}
      {isOpen && <ClickAwayArea onClick={() => setIsOpen(false)} />}
    </StyledWrapper>
  )
}

export default AccountFilterSelector
