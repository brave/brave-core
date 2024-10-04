// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { WalletRoutes } from '../../../constants/types'

// Selectors
import { UISelectors } from '../../../common/selectors'

// Components
import { DefaultPanelHeader } from './default-panel-header'

// Utils
import { getLocale } from '../../../../common/locale'

// Hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

import { AccountsMenu } from '../wallet-menus/accounts-menu'

// Styled Components
import {
  HeaderTitle,
  MenuButton,
  MenuButtonIcon,
  MenuWrapper
} from './shared-card-headers.style'
import { Row } from '../../shared/style'

export const AccountsHeader = () => {
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
    <DefaultPanelHeader
      title={getLocale('braveWalletTopNavAccounts')}
      expandRoute={WalletRoutes.Accounts}
    />
  ) : (
    <Row
      padding='24px 0px'
      justifyContent='space-between'
    >
      <HeaderTitle>{getLocale('braveWalletTopNavAccounts')}</HeaderTitle>
      <MenuWrapper ref={portfolioOverviewMenuRef}>
        <MenuButton
          onClick={() => setShowPortfolioOverviewMenu((prev) => !prev)}
        >
          <MenuButtonIcon name='plus-add' />
        </MenuButton>
        {showPortfolioOverviewMenu && <AccountsMenu />}
      </MenuWrapper>
    </Row>
  )
}

export default AccountsHeader
