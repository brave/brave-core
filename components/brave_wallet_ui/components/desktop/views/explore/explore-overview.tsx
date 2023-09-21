// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, Switch } from 'react-router'

// utils
import { ExploreNavOptions } from '../../../../options/nav-options'

// components
import SegmentedControl from '../../../shared/segmented-control/segmented-control'
import { WalletRoutes } from '../../../../constants/types'
import WalletPageWrapper from '../../wallet-page-wrapper/wallet-page-wrapper'
import PortfolioAsset from '../portfolio/portfolio-asset'
import { MarketView } from '../market'
import { ExploreWeb3 } from './components/explore-web3'

// styles
import { Column } from '../../../shared/style'
import { ControlsRow } from './explore-overview.styles'

export const ExploreOverview = () => {
  return (
    <Column
      fullWidth={true}
      justifyContent='flex-start'
      margin='0px 0px 15px 0px'
    >
      <ControlsRow>
        <SegmentedControl navOptions={ExploreNavOptions} width={384} />
      </ControlsRow>

      <Switch>
        <Route path={WalletRoutes.ExploreMarket} exact={true}>
          <MarketView />
        </Route>

        <Route path={WalletRoutes.ExploreMarketSub} exact={true}>
          <WalletPageWrapper wrapContentInBox={true}>
            <PortfolioAsset isShowingMarketData={true} />
          </WalletPageWrapper>
        </Route>

        <Route path={WalletRoutes.ExploreWeb3} exact={true}>
          <ExploreWeb3 />
        </Route>
      </Switch>
    </Column>
  )
}
