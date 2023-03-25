// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { WalletSelectors } from '../../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'

// Assets
import PersonIcon from '../../assets/person.svg'
import CaratDownIcon from '../../assets/carat-down-icon.svg'

// Hooks
import { useOnClickOutside } from '../../../../../common/hooks/useOnClickOutside'
import {
  useGetSelectedChainQuery //
} from '../../../../../common/slices/api.slice'

// Components
import { AccountListItem } from '../account-list-item/account-list-item'

// Styled Components
import { ButtonIcon, ArrowIcon, DropDown, SelectorButton } from './account-selector.style'

interface Props {
  onSelectAddress: (value: string) => void
  disabled: boolean
}

const ACCOUNT_SELECTOR_BUTTON_ID = 'account-selector-button-id'

export const AccountSelector = (props: Props) => {
  const { onSelectAddress, disabled } = props

  // Selectors
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // State
  const [showAccountSelector, setShowAccountSelector] = React.useState<boolean>(false)

  // Refs
  const accountSelectorRef = React.useRef<HTMLDivElement>(null)

  // Methods
  const toggleShowAccountSelector = React.useCallback(() => {
    setShowAccountSelector(prev => !prev)
  }, [])

  const handleOnSelectAccount = React.useCallback((address: string) => {
    setShowAccountSelector(false)
    onSelectAddress(address)
  }, [onSelectAddress])

  // Memos
  const accountsByNetwork = React.useMemo(() => {
    return accounts.filter((account) => account.coin === selectedNetwork?.coin && account.keyringId === selectedAccount?.keyringId)
  }, [accounts, selectedNetwork, selectedAccount])

  // Hooks
  useOnClickOutside(
    accountSelectorRef,
    () => setShowAccountSelector(false),
    showAccountSelector,
    ACCOUNT_SELECTOR_BUTTON_ID
  )

  return (
    <>
      <SelectorButton disabled={disabled} id={ACCOUNT_SELECTOR_BUTTON_ID} onClick={toggleShowAccountSelector}>
        <ButtonIcon id={ACCOUNT_SELECTOR_BUTTON_ID} icon={PersonIcon} size={16} />
        <ArrowIcon id={ACCOUNT_SELECTOR_BUTTON_ID} icon={CaratDownIcon} size={12} isOpen={showAccountSelector} />
      </SelectorButton>
      {showAccountSelector &&
        <DropDown ref={accountSelectorRef}>
          {accountsByNetwork.map((account) =>
            <AccountListItem
              key={account.address}
              onClick={handleOnSelectAccount}
              address={account.address}
              name={account.name}
            />
          )}
        </DropDown>
      }
    </>
  )
}

export default AccountSelector
