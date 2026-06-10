// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { NavOption } from '../../../../../../constants/types'

// Hooks
import { useRoute } from '../../../../../../common/hooks/use_route'

// Options
import {
  BuySendSwapDepositOptions,
  BuySendSwapDepositIOSOptions,
} from '../../../../../../options/nav-options'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Selectors
import {
  useSafeUISelector, //
} from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// Components
import {
  PortfolioActionsMoreMenu, //
} from '../../../../wallet-menus/portfolio_actions_more_menu'

// Styled Components
import { Button, ButtonIcon } from './buy-send-swap-deposit-nav.style'
import { Row, Column, Text } from '../../../../../shared/style'

export const BuySendSwapDepositNav = () => {
  // selectors
  const isIOS = useSafeUISelector(UISelectors.isIOS)

  // hooks
  const { openOrPushRoute } = useRoute()

  // methods
  const onClick = React.useCallback(
    (option: NavOption) => {
      openOrPushRoute(option.route)
    },
    [openOrPushRoute],
  )

  const options = isIOS
    ? BuySendSwapDepositIOSOptions
    : BuySendSwapDepositOptions

  return (
    <Row
      width='unset'
      gap='28px'
    >
      {options.slice(0, 3).map((option) => (
        <Column key={option.id}>
          <Button onClick={() => onClick(option)}>
            <ButtonIcon name={option.icon} />
          </Button>
          <Text
            textColor='primary'
            variant='small.semibold'
          >
            {getLocale(option.name)}
          </Text>
        </Column>
      ))}
      <PortfolioActionsMoreMenu onClick={onClick}>
        <Column slot='anchor-content'>
          <Button>
            <ButtonIcon name='more-horizontal' />
          </Button>
          <Text
            textColor='primary'
            variant='small.semibold'
          >
            {getLocale('braveWalletButtonMore')}
          </Text>
        </Column>
      </PortfolioActionsMoreMenu>
    </Row>
  )
}

export default BuySendSwapDepositNav
