// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, Route, Switch, useHistory } from 'react-router'

// components
import {
  OnboardingWelcome,
  OnboardingCreatePassword,
  OnboardingImportMetaMaskOrLegacy
} from '../../components/desktop'
import { BackButton } from '../../components/shared'
import BackupWallet from './backup-wallet'

// types
import { WalletRoutes } from '../../constants/types'

// style
import { OnboardingWrapper } from '../style'

// actions
import * as WalletPageActions from '../../page/actions/wallet_page_actions'

export const Onboarding = () => {
  // routing
  let history = useHistory()

  // redux
  const dispatch = useDispatch()

  // methods
  const onSkipBackup = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete(true))
    history.push(WalletRoutes.Portfolio)
  }, [])

  // effects
  React.useEffect(() => {
    dispatch(WalletPageActions.onOnboardingShown())
    dispatch(WalletPageActions.checkWalletsToImport())
  }, [])

  // render
  return (
    <OnboardingWrapper>
      <Switch>

        <Route path={WalletRoutes.OnboardingCreatePassword} exact>
          <BackButton onSubmit={history.goBack} />
          <OnboardingCreatePassword />
        </Route>

        <Route path={WalletRoutes.OnboardingBackupWallet} exact>
          <BackupWallet
            isOnboarding={true}
            onCancel={onSkipBackup}
          />
        </Route>

        <Route path={WalletRoutes.OnboardingImportMetaMask} exact>
          <BackButton onSubmit={history.goBack} />
          <OnboardingImportMetaMaskOrLegacy />
        </Route>

        <Route path={WalletRoutes.OnboardingImportCryptoWallets} exact>
          <BackButton onSubmit={history.goBack} />
          <OnboardingImportMetaMaskOrLegacy />
        </Route>

        <Route path={WalletRoutes.Onboarding} exact>
          <OnboardingWelcome />
        </Route>

        <Redirect to={WalletRoutes.Onboarding} />

      </Switch>
    </OnboardingWrapper>
  )
}

export default Onboarding
