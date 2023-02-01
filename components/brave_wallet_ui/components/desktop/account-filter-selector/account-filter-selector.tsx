// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Redux
import {
  useSelector,
  useDispatch
} from 'react-redux'
import { WalletActions } from '../../../common/actions'

// Types
import { BraveWallet, WalletAccountType, WalletState } from '../../../constants/types'
import { LOCAL_STORAGE_KEYS } from '../../../common/constants/local-storage-keys'

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

interface Props {
  onSelectAccount?: (account: Pick<WalletAccountType, 'address' | 'name'>) => void
  selectedAccount?: Pick<WalletAccountType, 'address' | 'name'>
  selectedNetwork?: BraveWallet.NetworkInfo
}

export const AccountFilterSelector = ({
  onSelectAccount,
  selectedAccount: accountProp,
  selectedNetwork: networkProp
}: Props) => {
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
    if (onSelectAccount) {
      onSelectAccount(account)
      return
    }
    window.localStorage.setItem(LOCAL_STORAGE_KEYS.PORTFOLIO_ACCOUNT_FILTER_OPTION, account.id)
    dispatch(WalletActions.setSelectedAccountFilterItem(account.id))
  }, [onSelectAccount])

  // Memos
  const selectedAccount = accountProp || [...accounts, AllAccountsOption].find(account => account.id === selectedAccountFilter) || AllAccountsOption
  const selectedNetwork = networkProp || selectedNetworkFilter

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  // Filters accounts by network if a selectedNetworkFilter is selected
  const accountsFilteredBySelectedNetworkFilter: WalletAccountType[] = React.useMemo(() => {
    return selectedNetwork.chainId === AllNetworksOption.chainId
      ? accounts
      : accounts.filter((account) => account.coin === selectedNetwork.coin)
  }, [accounts, selectedNetwork])

  const accountsList: WalletAccountType[] = React.useMemo(() => {
    return [AllAccountsOption, ...accountsFilteredBySelectedNetworkFilter]
  }, [accountsFilteredBySelectedNetworkFilter])

  return (
    <StyledWrapper>
      <DropDownButton onClick={onClick}>
        <SelectorLeftSide>
          {selectedAccount.address !== AllAccountsOption.address &&
            <AccountCircle orb={orb} />
          }
          {selectedAccount.name}
        </SelectorLeftSide>
        <DropDownIcon />
      </DropDownButton>
      {isOpen &&
        <DropDown>
          {accountsList.map(account =>
            <AccountFilterItem
              key={account.address}
              account={account}
              selected={account.address === selectedAccount.address}
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
