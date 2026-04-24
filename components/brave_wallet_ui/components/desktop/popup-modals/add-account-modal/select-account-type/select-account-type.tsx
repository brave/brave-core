// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// utils
import { getLocale } from '$web-common/locale'

// types
import {
  CreateAccountOptionsType,
  SupportedTestNetworks,
} from '../../../../../constants/types'

// components
import AccountTypeItem from '../account-type-item'

// style
import {
  SectionWrapper,
  SelectAccountItemWrapper,
  SelectAccountTitle,
  TestAccountsButton,
  StyledWrapper,
} from './select-account-type.style'

interface Props {
  createAccountOptions: CreateAccountOptionsType[]
  onSelectAccountType: (accountType: CreateAccountOptionsType) => () => void
}

export const SelectAccountType = ({
  createAccountOptions,
  onSelectAccountType,
}: Props) => {
  // State
  const [showTestnetAccounts, setShowTestnetAccounts] = React.useState(false)

  // Memos
  const mainnetAccountOptions = React.useMemo(
    () =>
      createAccountOptions.filter(
        (option) => !SupportedTestNetworks.includes(option.fixedNetwork ?? ''),
      ),
    [createAccountOptions],
  )

  const testnetAccountOptions = React.useMemo(
    () =>
      createAccountOptions.filter((option) =>
        SupportedTestNetworks.includes(option.fixedNetwork ?? ''),
      ),
    [createAccountOptions],
  )

  // render
  return (
    <StyledWrapper
      gap='16px'
      justifyContent='flex-start'
      alignItems='flex-start'
    >
      <SelectAccountTitle textColor='primary'>
        {getLocale('braveWalletCreateAccountTitle')}
      </SelectAccountTitle>

      <SectionWrapper width='100%'>
        {mainnetAccountOptions.map((network) => (
          <SelectAccountItemWrapper
            width='100%'
            key={network.name}
          >
            <AccountTypeItem
              onClickCreate={onSelectAccountType(network)}
              icon={network.icon}
              description={network.description}
              title={network.name}
            />
          </SelectAccountItemWrapper>
        ))}
      </SectionWrapper>

      {testnetAccountOptions.length > 0 && (
        <SectionWrapper width='100%'>
          <TestAccountsButton
            isOpen={showTestnetAccounts}
            onClick={() => setShowTestnetAccounts(!showTestnetAccounts)}
          >
            {getLocale('braveWalletLookingForTestnetAccounts')}
            <Icon name={showTestnetAccounts ? 'carat-up' : 'carat-down'} />
          </TestAccountsButton>
          {showTestnetAccounts
            && testnetAccountOptions.map((network, index) => (
              <SelectAccountItemWrapper
                width='100%'
                key={network.name}
              >
                <AccountTypeItem
                  onClickCreate={onSelectAccountType(network)}
                  icon={network.icon}
                  description={network.description}
                  title={network.name}
                />
              </SelectAccountItemWrapper>
            ))}
        </SectionWrapper>
      )}
    </StyledWrapper>
  )
}
