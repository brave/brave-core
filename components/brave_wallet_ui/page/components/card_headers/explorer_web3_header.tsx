// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { WalletRoutes } from '$wallet/constants/types'

// Selectors
import { UISelectors } from '$wallet/common/selectors'

// Components
import { DefaultPanelHeader } from './default_panel_header'

// Utils
import { getLocale } from '$web-common/locale'

// Hooks
import { useSafeUISelector } from '$wallet/common/hooks/use-safe-selector'

// Styled Components
import { HeaderTitle } from './shared_card_headers.style'
import { Row } from '$wallet/components/shared/style'

export const ExploreWeb3Header = () => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)

  return isPanel || isMobile ? (
    <DefaultPanelHeader
      title={getLocale(S.BRAVE_WALLET_TOP_NAV_EXPLORE)}
      expandRoute={WalletRoutes.Explore}
    />
  ) : (
    <Row
      padding='24px 0px'
      justifyContent='flex-start'
    >
      <HeaderTitle
        textColor='primary'
        variant='heading.h1'
      >
        {getLocale(S.BRAVE_WALLET_TOP_NAV_EXPLORE)}
      </HeaderTitle>
    </Row>
  )
}
