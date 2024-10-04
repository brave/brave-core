// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router-dom'

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
import {
  HeaderTitle,
  MenuButton,
  MenuButtonIcon
} from './shared-card-headers.style'
import { Row } from '../../shared/style'

export const PortfolioOverviewHeader = () => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Routing
  const history = useHistory()

  return isPanel ? (
    <DefaultPanelHeader title={getLocale('braveWalletTopNavPortfolio')} />
  ) : (
    <Row
      padding='24px 0px'
      justifyContent='space-between'
    >
      <HeaderTitle>{getLocale('braveWalletTopNavPortfolio')}</HeaderTitle>
      {/* ToDo: Route to Add Token or Add NFT, to be handled in https://github.com/brave/brave-browser/issues/37258 */}
      <MenuButton onClick={() => history.push(WalletRoutes.AddAssetModal)}>
        <MenuButtonIcon name='plus-add' />
      </MenuButton>
    </Row>
  )
}

export default PortfolioOverviewHeader
