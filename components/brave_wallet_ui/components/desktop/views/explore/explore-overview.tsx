// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, Switch } from 'react-router'

// utils
import { ExploreNavOptions } from '../../../../options/nav-options'
import { WalletRoutes } from '../../../../constants/types'

// components
import SegmentedControl from '../../../shared/segmented-control/segmented-control'
import { MarketView } from '../market'
import { ExploreDapps } from './components/explore-dapps'

// styles
import { Column } from '../../../shared/style'
import { ControlsRow } from './explore-overview.styles'
import PortfolioAsset from '../portfolio/portfolio-asset'

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
          <PortfolioAsset isShowingMarketData={true} />
        </Route>

        <Route path={WalletRoutes.ExploreDapps} exact={true}>
          <ExploreDapps />
        </Route>

        <Route
          path={WalletRoutes.Explore}
          exact={true}
          render={() => <Redirect to={WalletRoutes.ExploreMarket} />}
        />
      </Switch>
    </Column>
  )
}
