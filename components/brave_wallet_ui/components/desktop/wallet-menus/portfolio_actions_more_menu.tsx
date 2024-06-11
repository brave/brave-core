// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { NavOption } from '../../../constants/types'

// Options
import { BuySendSwapDepositOptions } from '../../../options/nav-options'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'

interface Props {
  onClick: (option: NavOption) => void
}

export const PortfolioAccountMenu = (props: Props) => {
  const { onClick } = props

  return (
    <StyledWrapper yPosition={56}>
      {BuySendSwapDepositOptions.slice(3).map((option) => (
        <PopupButton
          key={option.id}
          onClick={() => onClick(option)}
        >
          <ButtonIcon name={option.icon} />
          <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
        </PopupButton>
      ))}
    </StyledWrapper>
  )
}
