// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Queries
import {
  useAccountFromAddressQuery //
} from '../../../../common/slices/api.slice.extra'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import {
  CreateAccountIcon //
} from '../../../../components/shared/create-account-icon/create-account-icon'

// Styled Components
import { SelectButton } from './select_address_button.style'
import { Row } from '../../../../components/shared/style'
import { CaratIcon, ButtonText } from '../shared_composer.style'

interface Props {
  onClick: () => void
  isDisabled: boolean
  toAddressOrUrl: string
}

export const SelectAddressButton = (props: Props) => {
  const { onClick, toAddressOrUrl, isDisabled } = props

  // Queries
  const { account: foundAccount } = useAccountFromAddressQuery(
    toAddressOrUrl ?? undefined
  )

  return (
    <SelectButton
      isPlaceholder={!toAddressOrUrl}
      onClick={onClick}
      disabled={isDisabled}
    >
      <Row width='unset'>
        {foundAccount && (
          <CreateAccountIcon
            size='big'
            account={foundAccount}
            marginRight={8}
          />
        )}
        <ButtonText
          textSize='22px'
          textAlign='left'
        >
          {toAddressOrUrl !== ''
            ? toAddressOrUrl
            : getLocale('braveWalletChooseRecipient')}
        </ButtonText>
      </Row>
      <CaratIcon />
    </SelectButton>
  )
}
