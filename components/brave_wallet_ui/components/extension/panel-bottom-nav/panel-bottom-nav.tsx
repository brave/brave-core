// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { PanelTypes, NavOption } from '../../../constants/types'

// Options
import { NavOptions } from '../../../options/nav-options'

// Components
import { PanelBottomNavButton } from './panel-bottom-nav-button/panel-bottom-nav-button'

// Styled Components
import {
  StyledWrapper,
  NavOutline
} from './panel-bottom-nav.style'

interface Props {
  onNavigate: (path: PanelTypes) => void
}

// Portfolio is not an option in the panel
const buttonOptions = NavOptions.filter((option) => option.id !== 'portfolio')

export const PanelBottomNav = (props: Props) => {
  const { onNavigate } = props

  const handleOnClick = React.useCallback((option: NavOption) => {
    if (option.id === 'transactions') {
      onNavigate('transactions')
      return
    }
    const url = `brave://wallet${option.route}`
    chrome.tabs.create({ url: url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [onNavigate])

  return (
    <StyledWrapper>
      <NavOutline>
        {buttonOptions.map((option) =>
          <PanelBottomNavButton
            key={option.id}
            onClick={() => handleOnClick(option)}
            option={option}
          />
        )}
      </NavOutline>
    </StyledWrapper>
  )
}

export default PanelBottomNav
