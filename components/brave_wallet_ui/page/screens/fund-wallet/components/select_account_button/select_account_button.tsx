// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Hooks
import {
  useReceiveAddressQuery //
} from '../../../../../common/slices/api.slice.extra'

// Components
import {
  CreateAccountIcon //
} from '../../../../../components/shared/create-account-icon/create-account-icon'

// Styled Components
import { Column, Row } from '../../../../../components/shared/style'
import { CaretDown, ControlText, Label, WrapperButton } from '../shared/style'
import { AccountAddress } from './select_account_button.style'

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
  const { receiveAddress } = useReceiveAddressQuery(selectedAccount?.accountId)

  return (
    <Column alignItems='flex-start'>
      <WrapperButton onClick={onClick}>
        <Column alignItems='flex-start'>
          <Label>{labelText}</Label>
          <Row
            justifyContent='flex-start'
            gap='8px'
            minHeight='40px'
            margin='0px 0px 12px 0px'
          >
            <CreateAccountIcon
              account={selectedAccount}
              size='medium'
            />
            <ControlText>{selectedAccount?.name}</ControlText>
            <CaretDown />
          </Row>
        </Column>
      </WrapperButton>
      <AccountAddress>{receiveAddress}</AccountAddress>
    </Column>
  )
}
