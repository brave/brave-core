// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Types
import { NavOption } from '../../../constants/types'

// Options
import {
  BuySendSwapDepositOptions,
  BuySendSwapDepositIOSOptions,
} from '../../../options/nav-options'

// Utils
import { getLocale } from '../../../../common/locale'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

interface Props {
  children: React.ReactNode
  onClick: (option: NavOption) => void
}

export const PortfolioActionsMoreMenu = (props: Props) => {
  const { onClick, children } = props

  // selectors
  const isIOS = useSafeUISelector(UISelectors.isIOS)

  // computed
  const options = isIOS
    ? BuySendSwapDepositIOSOptions
    : BuySendSwapDepositOptions

  return (
    <ButtonMenu placement='bottom-end'>
      {children}
      {options.slice(3).map((option) => (
        <leo-menu-item
          key={option.id}
          onClick={() => onClick(option)}
        >
          <Icon name={option.icon} />
          {getLocale(option.name)}
        </leo-menu-item>
      ))}
    </ButtonMenu>
  )
}
