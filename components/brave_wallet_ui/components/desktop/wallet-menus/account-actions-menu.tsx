// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Types
import {
  AccountModalTypes,
  AccountButtonOptionsObjectType
} from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'
import { VerticalDivider, VerticalSpace } from '../../shared/style'

export interface Props {
  options: AccountButtonOptionsObjectType[]
  onClick: (id: AccountModalTypes) => void
}

export const AccountActionsMenu = (props: Props) => {
  const { options, onClick } = props

  // Computed
  const minButtonWidth = options.some((option) => option.id === 'shield')
    ? 260
    : undefined

  return (
    <StyledWrapper yPosition={26}>
      {options.slice(0, 2).map((option) => (
        <PopupButton
          key={option.id}
          onClick={() => onClick(option.id)}
          minWidth={minButtonWidth}
        >
          <ButtonIcon
            name={option.icon}
            id={option.id}
          />
          <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
        </PopupButton>
      ))}
      <VerticalDivider />
      <VerticalSpace space='8px' />
      {options.slice(2).map((option) => (
        <PopupButton
          key={option.id}
          onClick={() => onClick(option.id)}
          minWidth={minButtonWidth}
        >
          <ButtonIcon
            name={option.icon}
            id={option.id}
          />
          <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
        </PopupButton>
      ))}
    </StyledWrapper>
  )
}

export default AccountActionsMenu
