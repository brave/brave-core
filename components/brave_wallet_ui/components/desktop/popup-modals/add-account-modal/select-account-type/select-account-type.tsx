// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// utils
import { getLocale } from '$web-common/locale'

// options
import { CreateAccountOptions } from '../../../../../options/create-account-options'

// types
import { BraveWallet, CreateAccountOptionsType, WalletState } from '../../../../../constants/types'

// components
import { DividerLine } from '../../../../extension'
import AccountTypeItem from '../account-type-item'

// style
import {
  SelectAccountItemWrapper,
  SelectAccountTitle,
  SelectAccountTypeWrapper
} from './select-account-type.style'

interface Props {
  onSelectAccountType: (accountType: CreateAccountOptionsType) => () => void
  buttonText: string
}

export const SelectAccountType = ({ buttonText, onSelectAccountType }: Props) => {
  // redux
  const { isSolanaEnabled, isFilecoinEnabled } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // render
  return (
    <SelectAccountTypeWrapper>

      <SelectAccountTitle>{getLocale('braveWalletCreateAccountTitle')}</SelectAccountTitle>

      <DividerLine />

      {CreateAccountOptions(isFilecoinEnabled, isSolanaEnabled).map((network) => (
        <SelectAccountItemWrapper key={network.coin}>

          <AccountTypeItem
            onClickCreate={onSelectAccountType(network)}
            icon={network.icon}
            description={network.description}
            title={network.name}
            buttonText={buttonText}
          />

          {network.coin !== BraveWallet.CoinType.FIL && <DividerLine />}

        </SelectAccountItemWrapper>
      ))}

    </SelectAccountTypeWrapper>
  )
}
