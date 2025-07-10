// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

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
  const isAndroid = useSafeUISelector(UISelectors.isAndroid)

  // Routing
  const history = useHistory()

  // Methods
  const onClose = () => {
    if (history.length > 1) {
      history.goBack()
      return
    }
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
        {!isAndroid && (
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
      <HeaderTitle isAndroidOrPanel={true}>{title}</HeaderTitle>
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
