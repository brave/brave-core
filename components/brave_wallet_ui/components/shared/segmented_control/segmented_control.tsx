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
import { ButtonsContainer, Button } from './segmented_control.style'

interface Props {
  navOptions: NavOption[]
  width?: number
}

export const SegmentedControl = (props: Props) => {
  const { navOptions, width } = props

  // Routing
  const history = useHistory()
  const { pathname: walletLocation, hash } = useLocation()

  // Methods
  const onClick = React.useCallback(
    (navOption: NavOption) => {
      history.push(navOption.route)
    },
    [history]
  )

  const isSelected = React.useCallback(
    (navOption: NavOption) => {
      return navOption.route.startsWith('#')
        ? !hash
          ? // Since hashes are not necessary
            // we default the first option as selected,
            // if there is no hash in the route location.
            navOptions[0].id === navOption.id
          : hash === navOption.route
        : walletLocation.includes(navOption.route)
    },
    [walletLocation, navOptions, hash]
  )

  return (
    <ButtonsContainer width={width}>
      {navOptions.map((navOption: NavOption) => (
        <Button
          key={navOption.id}
          isSelected={isSelected(navOption)}
          onClick={() => onClick(navOption)}
        >
          {getLocale(navOption.name)}
        </Button>
      ))}
    </ButtonsContainer>
  )
}

export default SegmentedControl
