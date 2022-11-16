// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

// Utils
import { getLocale } from '../../../../../common/locale'

// Types
import { BuySendSwapDepositOption } from '../../../../constants/types'

// Styled Components
import { StyledButton, ButtonIcon } from './buy-send-swap-deposit-button.style'

export interface Props {
  option: BuySendSwapDepositOption
}

export const BuySendSwapDepositButton = (props: Props) => {
  const { option } = props

  // Routing
  const history = useHistory()

  // Methods
  const onClick = React.useCallback(() => {
    history.push(option.route)
  }, [option.route])

  return (
    <StyledButton onClick={onClick}>
      <ButtonIcon icon={option.icon} />
      {getLocale(option.name)}
    </StyledButton>
  )
}

export default BuySendSwapDepositButton
