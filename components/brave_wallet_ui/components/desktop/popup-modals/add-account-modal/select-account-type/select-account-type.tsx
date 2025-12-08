// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '$web-common/locale'

// types
import {
  CreateAccountOptionsType,
  SupportedTestNetworks,
} from '../../../../../constants/types'

// components
import { DividerLine } from '../../../../extension/divider/index'
import AccountTypeItem from '../account-type-item'

// style
import {
  SelectAccountItemWrapper,
  SelectAccountTitle,
  SelectAccountTypeWrapper,
} from './select-account-type.style'
import { Text, Row } from '../../../../shared/style'

interface Props {
  createAccountOptions: CreateAccountOptionsType[]
  onSelectAccountType: (accountType: CreateAccountOptionsType) => () => void
  buttonText: string
}

export const SelectAccountType = ({
  createAccountOptions,
  buttonText,
  onSelectAccountType,
}: Props) => {
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
    <SelectAccountTypeWrapper>
      <SelectAccountTitle>
        {getLocale('braveWalletCreateAccountTitle')}
      </SelectAccountTitle>

      <DividerLine />

      {mainnetAccountOptions.map((network, index) => (
        <SelectAccountItemWrapper key={network.name}>
          <AccountTypeItem
            onClickCreate={onSelectAccountType(network)}
            icon={network.icon}
            description={network.description}
            title={network.name}
            buttonText={buttonText}
          />
          {index + 1 !== mainnetAccountOptions.length && <DividerLine />}
        </SelectAccountItemWrapper>
      ))}

      <Row
        margin='32px 0px 0px 0px'
        justifyContent='flex-start'
      >
        <Text
          textSize='12px'
          isBold={true}
          textColor='primary'
        >
          {getLocale('braveWalletTestnetAccounts')}
        </Text>
      </Row>

      {testnetAccountOptions.map((network, index) => (
        <SelectAccountItemWrapper key={network.name}>
          <AccountTypeItem
            onClickCreate={onSelectAccountType(network)}
            icon={network.icon}
            description={network.description}
            title={network.name}
            buttonText={buttonText}
          />

          {index + 1 !== testnetAccountOptions.length && <DividerLine />}
        </SelectAccountItemWrapper>
      ))}
    </SelectAccountTypeWrapper>
  )
}
