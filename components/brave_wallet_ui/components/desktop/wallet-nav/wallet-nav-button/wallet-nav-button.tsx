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
import { StyledButton, ButtonIcon, ButtonText } from './wallet-nav-button.style'

export interface Props {
  option: NavOption
}

export const WalletNavButton = (props: Props) => {
  const { option } = props

  // Routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // Methods
  const onClick = React.useCallback(() => {
    history.push(option.route)
  }, [option.route])

  return (
    <StyledButton
      onClick={onClick}
      isSelected={walletLocation.includes(option.route)}
    >
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
