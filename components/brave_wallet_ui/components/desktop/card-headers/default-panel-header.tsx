// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { WalletRoutes } from '../../../constants/types'

// Hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

// Utils
import { openWalletRouteTab } from '../../../utils/routes-utils'

// Components
import { WalletSettingsMenu } from '../wallet-menus/wallet_settings_menu'
import {
  DAppConnectionSettings //
} from '../../extension/dapp-connection-settings/dapp-connection-settings'

// Styled Components
import {
  Button,
  ButtonIcon,
  LeftRightContainer,
  ClickAwayArea
} from './shared-panel-headers.style'
import { HeaderTitle, MenuWrapper } from './shared-card-headers.style'
import { Row } from '../../shared/style'

interface Props {
  title: string
  expandRoute?: WalletRoutes
}

export const DefaultPanelHeader = (props: Props) => {
  const { title, expandRoute } = props

  // State
  const [showSettingsMenu, setShowSettingsMenu] = React.useState<boolean>(false)

  // Refs
  const settingsMenuRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    settingsMenuRef,
    () => setShowSettingsMenu(false),
    showSettingsMenu
  )

  // Methods
  const onClickExpand = React.useCallback(() => {
    if (expandRoute) {
      openWalletRouteTab(expandRoute)
      return
    }
    openWalletRouteTab(WalletRoutes.PortfolioAssets)
  }, [expandRoute])

  return (
    <Row
      padding='12px 16px'
      justifyContent='space-between'
    >
      <LeftRightContainer
        width='unset'
        justifyContent='flex-start'
      >
        <Button onClick={onClickExpand}>
          <ButtonIcon name='expand' />
        </Button>
      </LeftRightContainer>
      <HeaderTitle isPanel={true}>{title}</HeaderTitle>
      <LeftRightContainer
        width='unset'
        justifyContent='flex-end'
      >
        <DAppConnectionSettings />
        <MenuWrapper ref={settingsMenuRef}>
          <Button onClick={() => setShowSettingsMenu((prev) => !prev)}>
            <ButtonIcon name='more-vertical' />
          </Button>
          {showSettingsMenu && <WalletSettingsMenu />}
        </MenuWrapper>
      </LeftRightContainer>
      {showSettingsMenu && <ClickAwayArea />}
    </Row>
  )
}

export default DefaultPanelHeader
