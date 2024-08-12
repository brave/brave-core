// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router-dom'
import ControlItem from '@brave/leo/react/controlItem'

// Types
import { NavOption } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  SegmentedControl as StyledSegmentedControl,
  ControlItemWrapper,
  StyledWrapper
} from './segmented_control.style'

interface Props {
  navOptions: NavOption[]
  width?: string
}

export const SegmentedControl = ({ navOptions, width }: Props) => {
  // Routing
  const history = useHistory()
  const { pathname: walletLocation, hash } = useLocation()

  // Computed
  const selectedRoute =
    navOptions.find((option) =>
      option.route.startsWith('#')
        ? option.route === hash
        : option.route === walletLocation
    )?.route || navOptions[0].route

  // Render
  return (
    <StyledWrapper width={width}>
      <StyledSegmentedControl
        value={selectedRoute}
        onChange={({ value }) => {
          if (value) {
            history.push(value)
          }
        }}
      >
        {navOptions.map((option) => (
          <ControlItem
            key={option.name}
            value={option.route}
          >
            <ControlItemWrapper>{getLocale(option.name)}</ControlItemWrapper>
          </ControlItem>
        ))}
      </StyledSegmentedControl>
    </StyledWrapper>
  )
}

export default SegmentedControl
