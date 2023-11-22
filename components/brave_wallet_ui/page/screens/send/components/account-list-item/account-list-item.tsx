// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet } from '../../../../../constants/types'

// Styled Components
import { Button, ButtonText } from './account-list-item.style'
import { Column } from '../../shared.styles'

// Hooks
import {
  CreateAccountIcon //
} from '../../../../../components/shared/create-account-icon/create-account-icon'

interface Props {
  account: BraveWallet.AccountInfo
  onClick: (account: BraveWallet.AccountInfo) => void
  isSelected: boolean
  accountAlias: string | undefined
}

export const AccountListItem = (props: Props) => {
  const { onClick, account, isSelected, accountAlias } = props

  return (
    <Button
      disabled={isSelected}
      onClick={() => onClick(account)}
    >
      <CreateAccountIcon
        size='medium'
        account={account}
        marginRight={16}
      />
      <Column
        horizontalAlign='flex-start'
        verticalAlign='center'
      >
        <ButtonText
          textColor='text03'
          textSize='12px'
          isBold={false}
          textAlign='left'
        >
          {account.name}
        </ButtonText>
        <ButtonText
          textColor='text01'
          textSize='12px'
          isBold={false}
          textAlign='left'
        >
          {account.address}
        </ButtonText>
        {accountAlias && accountAlias !== '' && (
          <ButtonText
            textColor='text02'
            textSize='12px'
            isBold={false}
            textAlign='left'
          >
            {accountAlias}
          </ButtonText>
        )}
      </Column>
    </Button>
  )
}

export default AccountListItem
