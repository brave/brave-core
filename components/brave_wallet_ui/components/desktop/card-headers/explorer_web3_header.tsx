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
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Styled Components
import { HeaderTitle } from './shared-card-headers.style'
import { Row } from '../../shared/style'

export const ExploreWeb3Header = () => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  return isPanel ? (
    <DefaultPanelHeader
      title={getLocale('braveWalletTopNavExplore')}
      expandRoute={WalletRoutes.Explore}
    />
  ) : (
    <Row
      padding='24px 0px'
      justifyContent='flex-start'
    >
      <HeaderTitle>{getLocale('braveWalletTopNavExplore')}</HeaderTitle>
    </Row>
  )
}
