// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { WalletAccountType } from '../../../constants/types'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { create } from 'ethereum-blockies'

// Styled Components
import {
  StyledWrapper,
  AccountAddressText,
  AccountNameText,
  NameAndAddressColumn,
  AccountCircle,
  LeftSide,
  UnSelectedCircle,
  SelectedIcon
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'

export interface Props {
  account: WalletAccountType
  isSelected: boolean
  action: () => void
}

const SelectAddress = (props: Props) => {
  const { account, isSelected, action } = props

  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  return (
    <StyledWrapper onClick={action}>
      <LeftSide>
        <AccountCircle orb={orb} />
        <NameAndAddressColumn>
          <AccountNameText>{reduceAccountDisplayName(account.name, 22)}</AccountNameText>
          <AccountAddressText>
            {reduceAddress(account.address)}
          </AccountAddressText>
        </NameAndAddressColumn>
      </LeftSide>
      {isSelected ? <SelectedIcon /> : <UnSelectedCircle />}
    </StyledWrapper>
  )
}

export default SelectAddress
