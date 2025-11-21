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
  BuySendSwapDepositOptions, //
} from '../../../../../../options/nav-options'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import {
  PortfolioAccountMenu, //
} from '../../../../wallet-menus/portfolio_actions_more_menu'

// Styled Components
import {
  Button,
  ButtonIcon,
  ButtonText,
  ButtonWrapper,
  MoreMenuWrapper,
} from './buy-send-swap-deposit-nav.style'
import { Row } from '../../../../../shared/style'

export const BuySendSwapDepositNav = () => {
  // state
  const [showMoreMenu, setShowMoreMenu] = React.useState<boolean>(false)

  // refs
  const moreMenuRef = React.useRef<HTMLDivElement>(null)

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

  return (
    <Row width='unset'>
      {BuySendSwapDepositOptions.slice(0, 3).map((option) => (
        <ButtonWrapper key={option.id}>
          <Button onClick={() => onClick(option)}>
            <ButtonIcon name={option.icon} />
          </Button>
          <ButtonText>{getLocale(option.name)}</ButtonText>
        </ButtonWrapper>
      ))}
      <MoreMenuWrapper ref={moreMenuRef}>
        <Button onClick={() => setShowMoreMenu(true)}>
          <ButtonIcon name='more-horizontal' />
        </Button>
        <ButtonText>{getLocale('braveWalletButtonMore')}</ButtonText>
        {showMoreMenu && <PortfolioAccountMenu onClick={onClick} />}
      </MoreMenuWrapper>
    </Row>
  )
}

export default BuySendSwapDepositNav
