// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import {
  Redirect,
  Route,
  Switch
  // ,useHistory
} from 'react-router'

// components
import { OnboardingRecoveryPhrase } from './backup-recovery-phrase/onboarding-backup-recovery-phrase'
import { OnboardingCreatePassword } from './create-password/onboarding-create-password'
import { OnboardingRecoveryPhraseExplainer } from './explain-recovery-phrase/explain-recovery-phrase'
import { OnboardingVerifyRecoveryPhrase } from './verify-recovery-phrase/verify-recovery-phrase'
import { OnboardingWelcome } from './welcome/onboarding-welcome'

// types
import { WalletRoutes } from '../../../constants/types'

// actions
import * as WalletPageActions from '../../actions/wallet_page_actions'
import { OnboardingSuccess } from './onboarding-success/onboarding-success'

export const OnboardingRoutes = () => {
  // routing
  // let history = useHistory()

  // redux
  const dispatch = useDispatch()

  // methods
  // const onSkipBackup = React.useCallback(() => {
  //   dispatch(WalletPageActions.walletSetupComplete())
  //   history.push(WalletRoutes.Portfolio)
  // }, [])

  // effects
  React.useEffect(() => {
    dispatch(WalletPageActions.checkWalletsToImport())
  }, [])

  // render
  return (
    <Switch>

      <Route path={WalletRoutes.OnboardingCreatePassword} exact>
        <OnboardingCreatePassword />
      </Route>
      
      <Route path={WalletRoutes.OnboardingExplainRecoveryPhrase} exact>
        <OnboardingRecoveryPhraseExplainer />
      </Route>
      
      <Route path={WalletRoutes.OnboardingBackupRecoveryPhrase} exact>
        <OnboardingRecoveryPhrase />
      </Route>

      <Route path={WalletRoutes.OnboardingVerifyRecoveryPhrase} exact>
        <OnboardingVerifyRecoveryPhrase />
      </Route>

      <Route path={WalletRoutes.OnboardingComplete} exact>
        <OnboardingSuccess />
      </Route>

      {/*<Route path={WalletRoutes.OnboardingImportMetaMask} exact>
        <OnboardingImportMetaMaskOrLegacy />
      </Route>

      <Route path={WalletRoutes.OnboardingImportCryptoWallets} exact>
        <OnboardingImportMetaMaskOrLegacy />
      </Route> */}

      <Route path={WalletRoutes.Onboarding} exact>
        <OnboardingWelcome />
      </Route>

      <Redirect to={WalletRoutes.Onboarding} />

    </Switch>
  )
}

export default OnboardingRoutes
