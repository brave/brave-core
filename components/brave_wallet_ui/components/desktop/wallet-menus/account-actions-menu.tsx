// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Utils
import { getLocale } from '../../../../common/locale'

// Types
import {
  AccountModalTypes,
  AccountButtonOptionsObjectType,
} from '../../../constants/types'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

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
    <ButtonMenu
      placement='bottom-end'
      minWidth={minButtonWidth}
    >
      <Button
        fab
        slot='anchor-content'
        kind='plain-faint'
        size='large'
      >
        <Icon name='more-vertical' />
      </Button>
      {options.slice(0, 2).map((option) => (
        <leo-menu-item
          key={option.id}
          onClick={() => onClick(option.id)}
        >
          <Icon
            name={option.icon}
            id={option.id}
          />
          {getLocale(option.name)}
        </leo-menu-item>
      ))}
      <hr />
      {options.slice(2).map((option) => (
        <leo-menu-item
          key={option.id}
          onClick={() => onClick(option.id)}
        >
          <Icon
            name={option.icon}
            id={option.id}
          />
          {getLocale(option.name)}
        </leo-menu-item>
      ))}
    </ButtonMenu>
  )
}

export default AccountActionsMenu
