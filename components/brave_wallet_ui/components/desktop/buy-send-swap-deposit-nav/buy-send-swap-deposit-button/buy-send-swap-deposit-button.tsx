// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

// Utils
import { getLocale } from '../../../../../common/locale'

// Types
import { NavOption } from '../../../../constants/types'

// Components
import { NavTooltip } from '../../../shared/nav-tooltip/nav-tooltip'

// Styled Components
import { StyledButton, ButtonIcon } from './buy-send-swap-deposit-button.style'

export interface Props {
  option: NavOption
  isTab?: boolean
  isSwap?: boolean
}

export const BuySendSwapDepositButton = (props: Props) => {
  const { option, isTab, isSwap } = props

  // State
  const [active, setActive] = React.useState(false)

  // Routing
  const history = useHistory()

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
      isTab={isTab}
      onClick={onClick}
    >
      <ButtonIcon isTab={isTab} icon={option.icon} />
      {!isTab && getLocale(option.name)}
      <NavTooltip
        text={getLocale(option.name)}
        orientation='right'
        distance={46}
        showTip={!!isTab && active}
        isSwap={isSwap}
      />
    </StyledButton>
  )
}

export default BuySendSwapDepositButton
