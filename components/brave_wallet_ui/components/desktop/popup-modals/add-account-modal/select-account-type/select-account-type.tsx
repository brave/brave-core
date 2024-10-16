// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '$web-common/locale'

// types
import { CreateAccountOptionsType } from '../../../../../constants/types'

// components
import { DividerLine } from '../../../../extension/divider/index'
import AccountTypeItem from '../account-type-item'

// style
import {
  SelectAccountItemWrapper,
  SelectAccountTitle,
  SelectAccountTypeWrapper
} from './select-account-type.style'

interface Props {
  createAccountOptions: CreateAccountOptionsType[]
  onSelectAccountType: (accountType: CreateAccountOptionsType) => () => void
  buttonText: string
}

export const SelectAccountType = ({
  createAccountOptions,
  buttonText,
  onSelectAccountType
}: Props) => {
  // render
  return (
    <SelectAccountTypeWrapper>
      <SelectAccountTitle>
        {getLocale('braveWalletCreateAccountTitle')}
      </SelectAccountTitle>

      <DividerLine />

      {createAccountOptions.map((network, index) => (
        <SelectAccountItemWrapper key={network.name}>
          <AccountTypeItem
            onClickCreate={onSelectAccountType(network)}
            icon={network.icon}
            description={network.description}
            title={network.name}
            buttonText={buttonText}
          />

          {index + 1 !== createAccountOptions.length && <DividerLine />}
        </SelectAccountItemWrapper>
      ))}
    </SelectAccountTypeWrapper>
  )
}
