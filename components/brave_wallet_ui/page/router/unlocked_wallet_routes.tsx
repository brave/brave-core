// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Route, Switch } from 'react-router'

// constants
import { WalletRoutes } from '../../constants/types'

// components
import { CryptoView } from '../../components/desktop/views/crypto'
import { WalletPageLayout } from '../../components/desktop/wallet-page-layout'
import {
  WalletSubViewLayout //
} from '../../components/desktop/wallet-sub-view-layout'
import {
  BackupWalletRoutes //
} from '../screens/backup-wallet/backup-wallet.routes'
import { DepositFundsScreen } from '../screens/fund-wallet/deposit-funds'
import { FundWalletScreen } from '../screens/fund-wallet/fund-wallet'
import { SimplePageWrapper } from '../screens/page-screen.styles'
import {
  OnboardingSuccess //
} from '../screens/onboarding/onboarding-success/onboarding-success'

export const UnlockedWalletRoutes = ({
  sessionRoute
}: {
  sessionRoute: WalletRoutes | undefined
}) => {
  // render
  return (
    <Switch>
      <Route
        path={WalletRoutes.OnboardingComplete}
        exact
      >
        <WalletPageLayout>
          <WalletSubViewLayout>
            <OnboardingSuccess />
          </WalletSubViewLayout>
        </WalletPageLayout>
      </Route>

      <Route path={WalletRoutes.Backup}>
        <WalletPageLayout>
          <WalletSubViewLayout>
            <SimplePageWrapper>
              <BackupWalletRoutes />
            </SimplePageWrapper>
          </WalletSubViewLayout>
        </WalletPageLayout>
      </Route>

      <Route path={WalletRoutes.FundWalletPageStart}>
        <FundWalletScreen />
      </Route>

      <Route path={WalletRoutes.DepositFundsPageStart}>
        <DepositFundsScreen />
      </Route>

      <Route path={WalletRoutes.CryptoPage}>
        <CryptoView sessionRoute={sessionRoute} />
      </Route>
    </Switch>
  )
}
