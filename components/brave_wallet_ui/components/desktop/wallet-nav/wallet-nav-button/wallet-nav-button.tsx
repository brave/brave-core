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

// Styled Components
import {
  StyledButton,
  ButtonIcon,
  ButtonText,
  SelectedIndicator
} from './wallet-nav-button.style'

export interface Props {
  option: NavOption
  onClick?: (() => void) | (() => Promise<void>)
}

export const WalletNavButton = (props: Props) => {
  const { option } = props

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // Methods
  const onClick = React.useCallback(() => {
    props.onClick?.()
    history.push(props.option.route)
  }, [history, props])

  return (
    <StyledButton
      onClick={onClick}
      isSelected={walletLocation.includes(option.route)}
    >
      <SelectedIndicator />
      <ButtonIcon name={option.icon} />
      <ButtonText
        textSize='14px'
        isBold={true}
      >
        {getLocale(option.name)}
      </ButtonText>
    </StyledButton>
  )
}

export default WalletNavButton
