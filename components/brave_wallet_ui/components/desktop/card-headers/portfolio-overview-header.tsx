// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Hooks
import {
  useOnClickOutside
} from '../../../common/hooks/useOnClickOutside'

import {
  PortfolioOverviewMenu
} from '../wallet-menus/portfolio-overview-menu'

// Styled Components
import {
  HeaderTitle,
  CircleButton,
  ButtonIcon,
  MenuWrapper
} from './shared-card-headers.style'
import { Row } from '../../shared/style'

export const PortfolioOverviewHeader = () => {
  // State
  const [showPortfolioOverviewMenu, setShowPortfolioOverviewMenu] =
    React.useState<boolean>(false)

  // Refs
  const portfolioOverviewMenuRef =
    React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    portfolioOverviewMenuRef,
    () => setShowPortfolioOverviewMenu(false),
    showPortfolioOverviewMenu
  )

  return (
    <Row
      padding='24px 0px'
      justifyContent='space-between'
    >
      <HeaderTitle>
        {getLocale('braveWalletTopNavPortfolio')}
      </HeaderTitle>
      <MenuWrapper
        ref={portfolioOverviewMenuRef}
      >
        <CircleButton
          onClick={
            () => setShowPortfolioOverviewMenu(prev => !prev)
          }
        >
          <ButtonIcon
            name='more-vertical'
          />
        </CircleButton>
        {showPortfolioOverviewMenu &&
          <PortfolioOverviewMenu />
        }
      </MenuWrapper>
    </Row>
  )
}

export default PortfolioOverviewHeader
