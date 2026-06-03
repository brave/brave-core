// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { NavOption } from '../../../../../../constants/types'

// Hooks
import {
  useOnClickOutside, //
} from '../../../../../../common/hooks/useOnClickOutside'
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
  PortfolioAccountMenu, //
} from '../../../../wallet-menus/portfolio_actions_more_menu'

// Styled Components
import {
  Button,
  ButtonIcon,
  ButtonWrapper,
  MoreMenuWrapper,
} from './buy-send-swap-deposit-nav.style'
import { Row, Text } from '../../../../../shared/style'

export const BuySendSwapDepositNav = () => {
  // state
  const [showMoreMenu, setShowMoreMenu] = React.useState<boolean>(false)

  // refs
  const moreMenuRef = React.useRef<HTMLDivElement>(null)

  // selectors
  const isIOS = useSafeUISelector(UISelectors.isIOS)

  // hooks
  useOnClickOutside(moreMenuRef, () => setShowMoreMenu(false), showMoreMenu)
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
    <Row width='unset'>
      {options.slice(0, 3).map((option) => (
        <ButtonWrapper key={option.id}>
          <Button onClick={() => onClick(option)}>
            <ButtonIcon name={option.icon} />
          </Button>
          <Text
            textColor='primary'
            variant='small.semibold'
          >
            {getLocale(option.name)}
          </Text>
        </ButtonWrapper>
      ))}
      <MoreMenuWrapper ref={moreMenuRef}>
        <Button onClick={() => setShowMoreMenu(true)}>
          <ButtonIcon name='more-horizontal' />
        </Button>
        <Text
          textColor='primary'
          variant='small.semibold'
        >
          {getLocale('braveWalletButtonMore')}
        </Text>
        {showMoreMenu && <PortfolioAccountMenu onClick={onClick} />}
      </MoreMenuWrapper>
    </Row>
  )
}

export default BuySendSwapDepositNav
