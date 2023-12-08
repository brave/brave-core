// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

// Types
import { NavOption, WalletRoutes } from '../../../../../../constants/types'

// Selectors
import { useSafeUISelector } from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// Options
import {
  BuySendSwapDepositOptions //
} from '../../../../../../options/nav-options'

// Utils
import { getLocale } from '../../../../../../../common/locale'

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

  // redux
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // methods
  const onClick = React.useCallback(
    (option: NavOption) => {
      // Redirect to full page view for buy page
      // until we have a panel view for that page.
      if (option.route === WalletRoutes.FundWalletPageStart && isPanel) {
        chrome.tabs.create({
          url: `brave://wallet${option.route}`
        })
      } else {
        history.push(option.route)
      }
    },
    [history, isPanel]
  )

  return (
    <ButtonsRow width='unset'>
      {BuySendSwapDepositOptions.map((option) => (
        <ButtonWrapper key={option.id}>
          <Button onClick={() => onClick(option)}>
            <ButtonIcon name={option.icon} />
          </Button>
          <ButtonText>{getLocale(option.name)}</ButtonText>
        </ButtonWrapper>
      ))}
    </ButtonsRow>
  )
}

export default BuySendSwapDepositNav
