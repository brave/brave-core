// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router-dom'

// Types
import { NavOption } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  ButtonsContainer,
  Button
} from './segmented-control.style'

interface Props {
  navOptions: NavOption[]
  width?: number
}

export const SegmentedControl = (props: Props) => {
  const { navOptions, width } = props

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // Methods
  const onClick = (navOption: NavOption) => {
    history.push(navOption.route)
  }

  return (
    <ButtonsContainer
      width={width}
    >
      {navOptions.map((navOption: NavOption) =>
        <Button
          key={navOption.id}
          isSelected={walletLocation.includes(navOption.route)}
          onClick={() => onClick(navOption)}
        >
          {getLocale(navOption.name)}
        </Button>
      )}
    </ButtonsContainer>
  )
}

export default SegmentedControl
