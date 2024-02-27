// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  AccountButtonOptionsObjectType,
  AccountModalTypes
} from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'
import { VerticalDivider, VerticalSpace } from '../../shared/style'

interface Props {
  options: AccountButtonOptionsObjectType[]
  onClickMenuOption: (option: AccountModalTypes) => void
}

export const AccountDetailsMenu = (props: Props) => {
  const { options, onClickMenuOption } = props

  return (
    <StyledWrapper yPosition={42}>
      {options.map((option) => (
        <React.Fragment key={option.id}>
          {option.id === 'privateKey' && (
            <>
              <VerticalDivider />
              <VerticalSpace space='8px' />
            </>
          )}
          <PopupButton onClick={() => onClickMenuOption(option.id)}>
            <ButtonIcon name={option.icon} />
            <PopupButtonText>{getLocale(option.name)}</PopupButtonText>
          </PopupButton>
        </React.Fragment>
      ))}
    </StyledWrapper>
  )
}
