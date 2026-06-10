// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Types
import {
  AccountButtonOptionsObjectType,
  AccountModalTypes,
} from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

interface Props {
  children: React.ReactNode
  options: AccountButtonOptionsObjectType[]
  onClickMenuOption: (option: AccountModalTypes) => void
}

export const AccountDetailsMenu = (props: Props) => {
  const { children, options, onClickMenuOption } = props

  return (
    <ButtonMenu placement='bottom-end'>
      {children}
      {options.map((option) => (
        <React.Fragment key={option.id}>
          {option.id === 'privateKey' && <hr />}
          <leo-menu-item onClick={() => onClickMenuOption(option.id)}>
            <Icon name={option.icon} />
            {getLocale(option.name)}
          </leo-menu-item>
        </React.Fragment>
      ))}
    </ButtonMenu>
  )
}
