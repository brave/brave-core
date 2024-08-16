// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { WalletRoutes } from '../../../constants/types'

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
  LeftRightContainer
} from './shared-panel-headers.style'
import { HeaderTitle } from './shared-card-headers.style'
import { Row } from '../../shared/style'

interface Props {
  title: string
  expandRoute?: WalletRoutes
}

export const DefaultPanelHeader = (props: Props) => {
  const { title, expandRoute } = props

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
      padding='18px 16px'
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
        <WalletSettingsMenu />
      </LeftRightContainer>
    </Row>
  )
}

export default DefaultPanelHeader
