// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { reduceAddress } from '../../../../../../utils/reduce-address'

// Components
import {
  CreateAccountIcon //
} from '../../../../../shared/create-account-icon/create-account-icon'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Styled Components
import { AccountButton } from './change_account_button.style'
import { Text, Row } from '../../../../../shared/style'

interface Props {
  account: BraveWallet.AccountInfo
  onClick: () => void
}

export const ChangeAccountButton = (props: Props) => {
  const { account, onClick } = props

  return (
    <AccountButton onClick={onClick}>
      <Row width='unset'>
        <CreateAccountIcon
          size='small'
          account={account}
          marginRight={12}
        />
        <Text
          textSize='14px'
          isBold={true}
          textColor='primary'
        >
          {account.name}
        </Text>
      </Row>
      <Text
        textSize='12px'
        isBold={false}
        textColor='tertiary'
      >
        {reduceAddress(account.accountId.address)}
      </Text>
    </AccountButton>
  )
}
