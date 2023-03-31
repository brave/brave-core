// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router-dom'

// Utils
import { getLocale } from '../../../../../common/locale'

// Types
import { NavOption } from '../../../../constants/types'

// Components
import { NavTooltip } from '../../../shared/nav-tooltip/nav-tooltip'

// Styled Components
import { StyledButton, ButtonIcon } from './wallet-nav-button.style'

export interface Props {
  option: NavOption
  isSwap?: boolean
}

export const WalletNavButton = (props: Props) => {
  const { option, isSwap } = props

  // State
  const [active, setActive] = React.useState(false)

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // Methods
  const onClick = React.useCallback(() => {
    history.push(option.route)
  }, [option.route])

  const showTip = React.useCallback(() => {
    setActive(true)
  }, [])

  const hideTip = React.useCallback(() => {
    setActive(false)
  }, [])

  return (
    <StyledButton
      onMouseEnter={showTip}
      onMouseLeave={hideTip}
      onClick={onClick}
      isSelected={walletLocation.includes(option.route)}
    >
      <ButtonIcon name={option.icon} />
      <NavTooltip
        text={getLocale(option.name)}
        orientation='right'
        distance={60}
        showTip={active}
        isSwap={isSwap}
      />
    </StyledButton>
  )
}

export default WalletNavButton
