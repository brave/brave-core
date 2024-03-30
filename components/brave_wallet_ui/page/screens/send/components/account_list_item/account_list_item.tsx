// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Hooks
import {
  CreateAccountIcon //
} from '../../../../../components/shared/create-account-icon/create-account-icon'

// Utils
import { reduceAddress } from '../../../../../utils/reduce-address'
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import {
  Button,
  NameAndBalanceText,
  AddressText,
  DisabledLabel
} from './account_list_item.style'
import { Column, Row } from '../../../../../components/shared/style'

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
        size='big'
        account={account}
        marginRight={16}
      />
      <Column
        justifyContent='center'
        alignItems='flex-start'
      >
        <Row
          width='100%'
          justifyContent='flex-start'
          alignItems='center'
        >
          <NameAndBalanceText
            textSize='14px'
            isBold={true}
            textAlign='left'
          >
            {account.name}
          </NameAndBalanceText>
          {isSelected && (
            <DisabledLabel>{getLocale('braveWalletFrom')}</DisabledLabel>
          )}
        </Row>
        <AddressText
          textSize='12px'
          isBold={false}
          textAlign='left'
        >
          {reduceAddress(account.address)}
        </AddressText>
        {accountAlias && accountAlias !== '' && (
          <AddressText
            textSize='12px'
            isBold={false}
            textAlign='left'
          >
            {accountAlias}
          </AddressText>
        )}
      </Column>
    </Button>
  )
}

export default AccountListItem
