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
  PopupButton,
  MenuItemIcon,
  ButtonMenu,
  MenuItemRow,
  INTERACTIVE_ICON_COLOR
} from './wallet_menus.style'
import { VerticalDivider, VerticalSpace } from '../../shared/style'
import { MenuButton } from '../card-headers/shared-card-headers.style'

interface Props {
  options: AccountButtonOptionsObjectType[]
  onClickMenuOption: (option: AccountModalTypes) => void
}

export const AccountDetailsMenu = ({ options, onClickMenuOption }: Props) => {
  // render
  return (
    <ButtonMenu>
      <div slot='anchor-content'>
        <MenuButton>
          <MenuItemIcon
            name='more-vertical'
            color={INTERACTIVE_ICON_COLOR}
          />
        </MenuButton>
      </div>
      {options.map((option) => (
        <React.Fragment key={option.id}>
          {option.id === 'privateKey' && (
            <>
              <VerticalDivider />
              <VerticalSpace space='8px' />
            </>
          )}
          <PopupButton onClick={() => onClickMenuOption(option.id)}>
            <MenuItemRow>
              <MenuItemIcon name={option.icon} />
              {getLocale(option.name)}
            </MenuItemRow>
          </PopupButton>
        </React.Fragment>
      ))}
    </ButtonMenu>
  )
}
