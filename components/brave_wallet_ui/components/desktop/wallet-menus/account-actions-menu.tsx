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
  PopupButton,
  MenuItemIcon,
  ButtonMenu,
  MenuItemRow,
  MoreVerticalIcon,
  MenuButton
} from './wallet_menus.style'
import { VerticalDivider, VerticalSpace } from '../../shared/style'

export interface Props {
  options: AccountButtonOptionsObjectType[]
  onClick: (id: AccountModalTypes) => void
}

export const AccountActionsMenu = ({ options, onClick }: Props) => {
  // render
  return (
    <ButtonMenu>
      <div slot='anchor-content'>
        <MenuButton
          kind='plain-faint'
          padding='0px'
        >
          <MoreVerticalIcon />
        </MenuButton>
      </div>
      {options.slice(0, 2).map((option) => (
        <PopupButton
          key={option.id}
          onClick={() => onClick(option.id)}
        >
          <MenuItemRow>
            <MenuItemIcon name={option.icon} />
            {getLocale(option.name)}
          </MenuItemRow>
        </PopupButton>
      ))}
      <VerticalDivider />
      <VerticalSpace space='8px' />
      {options.slice(2).map((option) => (
        <PopupButton
          key={option.id}
          onClick={() => onClick(option.id)}
        >
          <MenuItemRow>
            <MenuItemIcon name={option.icon} />
            {getLocale(option.name)}
          </MenuItemRow>
        </PopupButton>
      ))}
    </ButtonMenu>
  )
}

export default AccountActionsMenu
