// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import {
  useSafeUISelector, //
} from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Types
import { WalletRoutes } from '../../../constants/types'

// Utils
import { openWalletRouteTab } from '../../../utils/routes-utils'

// Components
import { WalletSettingsMenu } from '../wallet-menus/wallet_settings_menu'

// Styled Components
import {
  Button,
  ButtonIcon,
  LeftRightContainer,
} from './shared-panel-headers.style'
import { HeaderTitle } from './shared-card-headers.style'
import { Row } from '../../shared/style'

interface Props {
  title: string
  expandRoute?: WalletRoutes
  actionIconName?: string
  onClickActionButton?: () => void
}

export const DefaultPanelHeader = (props: Props) => {
  const { title, expandRoute, actionIconName, onClickActionButton } = props

  // UI Selectors (safe)
  const isMobile = useSafeUISelector(UISelectors.isMobile)

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
      padding='16px'
      justifyContent='space-between'
    >
      <LeftRightContainer
        width='unset'
        justifyContent='flex-start'
      >
        {!isMobile && expandRoute && (
          <Button onClick={onClickExpand}>
            <ButtonIcon name='expand' />
          </Button>
        )}
      </LeftRightContainer>
      <HeaderTitle
        variant='large.semibold'
        textColor='primary'
      >
        {title}
      </HeaderTitle>
      <LeftRightContainer
        width='unset'
        justifyContent='flex-end'
      >
        {actionIconName && onClickActionButton && (
          <Button onClick={onClickActionButton}>
            <ButtonIcon name={actionIconName} />
          </Button>
        )}
        <WalletSettingsMenu>
          <Button slot='anchor-content'>
            <ButtonIcon name='more-vertical' />
          </Button>
        </WalletSettingsMenu>
      </LeftRightContainer>
    </Row>
  )
}

export default DefaultPanelHeader
