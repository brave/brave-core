// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

// Options
import {
  BuySendSwapDepositOptions
} from '../../../../../../options/nav-options'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'

// Styled Components
import {
  Button,
  ButtonIcon,
  ButtonText,
  ButtonWrapper,
  ButtonsRow
} from './buy-send-swap-deposit-nav.style'

export const BuySendSwapDepositNav = () => {
  // Routing
  const history = useHistory()

  return (
    <ButtonsRow
      width='unset'
    >
      {BuySendSwapDepositOptions.map((option) =>
        <ButtonWrapper
          key={option.id}
        >
          <Button
            onClick={() => history.push(option.route)}
          >
            <ButtonIcon name={option.icon} />
          </Button>
          <ButtonText>
            {getLocale(option.name)}
          </ButtonText>
        </ButtonWrapper>
      )}
    </ButtonsRow>
  )
}

export default BuySendSwapDepositNav
