// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router-dom'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'

// Selectors
import {
  useSafeUISelector, //
} from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Types
import { WalletRoutes } from '../../../constants/types'

// Utils
import { openWalletRouteTab } from '../../../utils/routes-utils'

// Styled Components
import {
  Button,
  ButtonIcon,
  LeftRightContainer,
} from './shared-panel-headers.style'
import { Row } from '../../shared/style'
import { HeaderTitle } from './shared-card-headers.style'

interface Props {
  title: string
  expandRoute: WalletRoutes
  onBack?: () => void
}

export const PanelActionHeader = (props: Props) => {
  const { title, expandRoute, onBack } = props

  // UI Selectors (safe)
  const isMobile = useSafeUISelector(UISelectors.isMobile)

  const previousLocation = window.localStorage.getItem(
    LOCAL_STORAGE_KEYS.PREVIOUS_LOCATION_ROUTE,
  )

  // Routing
  const history = useHistory()
  const location = useLocation()

  // Methods
  const onClose = () => {
    if (previousLocation && !previousLocation.includes(location.pathname)) {
      history.push(previousLocation)
      return
    }

    // Default to portfolio assets
    history.push(WalletRoutes.PortfolioAssets)
  }

  return (
    <Row
      padding='18px 16px'
      justifyContent='space-between'
    >
      <LeftRightContainer
        width='unset'
        justifyContent='flex-start'
      >
        {!isMobile && (
          <Button onClick={() => openWalletRouteTab(expandRoute)}>
            <ButtonIcon name='expand' />
          </Button>
        )}
        {onBack && (
          <Button onClick={onBack}>
            <ButtonIcon name='arrow-left' />
          </Button>
        )}
      </LeftRightContainer>
      <HeaderTitle isMobileOrPanel={true}>{title}</HeaderTitle>
      <LeftRightContainer
        width='unset'
        justifyContent='flex-end'
      >
        <Button onClick={onClose}>
          <ButtonIcon name='close' />
        </Button>
      </LeftRightContainer>
    </Row>
  )
}
