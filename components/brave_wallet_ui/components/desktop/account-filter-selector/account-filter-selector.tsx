// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Redux
import {
  useSelector,
  useDispatch
} from 'react-redux'
import { WalletActions } from '../../../common/actions'

// Types
import { WalletAccountType, WalletState } from '../../../constants/types'

// Options
import { AllAccountsOption } from '../../../options/account-filter-options'
import { AllNetworksOption } from '../../../options/network-filter-options'

// Components
import { AccountFilterItem } from './account-filter-item'

// Styles
import {
  AccountCircle
} from './account-filter-selector.style'

import {
  StyledWrapper,
  DropDown,
  DropDownButton,
  DropDownIcon,
  SelectorLeftSide,
  ClickAwayArea
} from '../network-filter-selector/style'

export const AccountFilterSelector = () => {
  // Redux
  const dispatch = useDispatch()

  // Wallet State
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const selectedAccountFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedAccountFilter)
  const selectedNetworkFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetworkFilter)

  // State
  const [isOpen, setIsOpen] = React.useState(false)

  // Methods
  const onClick = React.useCallback(() => {
    setIsOpen(prevIsOpen => !prevIsOpen)
  }, [])

  const onSelectAccountAndClose = React.useCallback((account: WalletAccountType) => {
    setIsOpen(false)
    dispatch(WalletActions.setSelectedAccountFilterItem(account))
  }, [dispatch])

  // Memos
  const orb = React.useMemo(() => {
    return create({ seed: selectedAccountFilter.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccountFilter.address])

  // Filters accounts by network if a selectedNetworkFilter is selected
  const accountsFilteredBySelectedNetworkFilter: WalletAccountType[] = React.useMemo(() => {
    return selectedNetworkFilter.chainId === AllNetworksOption.chainId
      ? accounts
      : accounts.filter((account) => account.coin === selectedNetworkFilter.coin)
  }, [accounts, selectedNetworkFilter])

  const accountsList: WalletAccountType[] = React.useMemo(() => {
    return [AllAccountsOption, ...accountsFilteredBySelectedNetworkFilter]
  }, [accountsFilteredBySelectedNetworkFilter, AllAccountsOption])

  return (
    <StyledWrapper>
      <DropDownButton onClick={onClick}>
        <SelectorLeftSide>
          {selectedAccountFilter.address !== '' &&
            <AccountCircle orb={orb} />
          }
          {selectedAccountFilter.name}
        </SelectorLeftSide>
        <DropDownIcon />
      </DropDownButton>
      {isOpen &&
        <DropDown>
          {accountsList.map(account =>
            <AccountFilterItem
              key={account.address}
              account={account}
              selected={selectedAccountFilter.address === account.address}
              onSelectAccount={onSelectAccountAndClose}
            />
          )}
        </DropDown>
      }
      {isOpen &&
        <ClickAwayArea onClick={() => setIsOpen(false)} />
      }
    </StyledWrapper>
  )
}

export default AccountFilterSelector
