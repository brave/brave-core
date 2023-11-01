// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { UISelectors } from '../../../common/selectors'

// Components
import { DefaultPanelHeader } from './default-panel-header'

// Utils
import { getLocale } from '../../../../common/locale'

// Hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

import { PortfolioOverviewMenu } from '../wallet-menus/portfolio-overview-menu'

// Styled Components
import {
  HeaderTitle,
  CircleButton,
  ButtonIcon,
  MenuWrapper
} from './shared-card-headers.style'
import { Row } from '../../shared/style'

export const PortfolioOverviewHeader = () => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // State
  const [showPortfolioOverviewMenu, setShowPortfolioOverviewMenu] =
    React.useState<boolean>(false)

  // Refs
  const portfolioOverviewMenuRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    portfolioOverviewMenuRef,
    () => setShowPortfolioOverviewMenu(false),
    showPortfolioOverviewMenu
  )

  return isPanel ? (
    <DefaultPanelHeader title={getLocale('braveWalletTopNavPortfolio')} />
  ) : (
    <Row
      padding='24px 0px'
      justifyContent='space-between'
    >
      <HeaderTitle>{getLocale('braveWalletTopNavPortfolio')}</HeaderTitle>
      <MenuWrapper ref={portfolioOverviewMenuRef}>
        <CircleButton
          onClick={() => setShowPortfolioOverviewMenu((prev) => !prev)}
        >
          <ButtonIcon name='tune' />
        </CircleButton>
        {showPortfolioOverviewMenu && <PortfolioOverviewMenu />}
      </MenuWrapper>
    </Row>
  )
}

export default PortfolioOverviewHeader
