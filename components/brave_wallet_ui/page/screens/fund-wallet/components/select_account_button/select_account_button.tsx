// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../../../constants/types'

// components
import { CreateAccountIcon } from '../../../../../components/shared/create-account-icon/create-account-icon'

// styles
import { Column, Row } from '../../../../../components/shared/style'
import {
  CaretDown,
  ControlsWrapper,
  Label,
  WrapperButton
} from '../shared/style'
import { AccountAddress, AccountName } from './select_account_button.style'

interface SelectAccountProps {
  labelText: string
  selectedAccount?: BraveWallet.AccountInfo
  onClick: () => void
}

export const SelectAccountButton = ({
  labelText,
  selectedAccount,
  onClick
}: SelectAccountProps) => {
  return (
    <Column alignItems='flex-start'>
      <WrapperButton onClick={onClick}>
        <Column alignItems='flex-start'>
          <Label>{labelText}</Label>
          <ControlsWrapper>
            <Row
              justifyContent='flex-start'
              gap='8px'
            >
              <CreateAccountIcon
                account={selectedAccount}
                size='medium'
              />
              <AccountName>{selectedAccount?.name}</AccountName>
            </Row>
            <CaretDown />
          </ControlsWrapper>
        </Column>
      </WrapperButton>
      <AccountAddress>{selectedAccount?.address}</AccountAddress>
    </Column>
  )
}
