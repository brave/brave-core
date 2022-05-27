// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import {
  Redirect,
  Route,
  Switch
  // ,useHistory
} from 'react-router'

// components
import { OnboardingBackupRecoveryPhrase } from './backup-recovery-phrase/onboarding-backup-recovery-phrase'
import { OnboardingCreatePassword } from './create-password/onboarding-create-password'
import { OnboardingRecoveryPhraseExplainer } from './explain-recovery-phrase/explain-recovery-phrase'
import { OnboardingVerifyRecoveryPhrase } from './verify-recovery-phrase/verify-recovery-phrase'
import { OnboardingWelcome } from './welcome/onboarding-welcome'
import { OnboardingImportOrRestoreWallet } from './import-or-restore-wallet/import-or-restore-wallet'
import { OnboardingSuccess } from './onboarding-success/onboarding-success'
import { OnboardingRestoreFromRecoveryPhrase } from './restore-from-recovery-phrase/restore-from-recovery-phrase'

// types
import { PageState, WalletRoutes } from '../../../constants/types'

// actions
// import * as WalletPageActions from '../../actions/wallet_page_actions'

export const OnboardingRoutes = () => {
  // routing
  // let history = useHistory()

  // redux
  const isMetaMaskInitialized = useSelector(({ page }: { page: PageState }) => page.isMetaMaskInitialized)

  // methods
  // const onSkipBackup = React.useCallback(() => {
  //   dispatch(WalletPageActions.walletSetupComplete())
  //   history.push(WalletRoutes.Portfolio)
  // }, [])

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
        <OnboardingBackupRecoveryPhrase />
      </Route>

      <Route path={WalletRoutes.OnboardingVerifyRecoveryPhrase} exact>
        <OnboardingVerifyRecoveryPhrase />
      </Route>

      <Route path={WalletRoutes.OnboardingImportOrRestore} exact>
        <OnboardingImportOrRestoreWallet />
      </Route>

      <Route path={WalletRoutes.OnboardingRestoreWallet} exact>
        <OnboardingRestoreFromRecoveryPhrase restoreFrom='seed' />
      </Route>

      <Route path={WalletRoutes.OnboardingImportMetaMask} exact>
        <OnboardingRestoreFromRecoveryPhrase
          restoreFrom={isMetaMaskInitialized ? 'metamask' : 'metamask-seed'}
        />
      </Route>

      {/*
      <Route path={WalletRoutes.OnboardingImportCryptoWallets} exact>
        <OnboardingImportMetaMaskOrLegacy />
      </Route>
      */}

      <Route path={WalletRoutes.OnboardingComplete} exact>
        <OnboardingSuccess />
      </Route>

      <Route path={WalletRoutes.OnboardingWelcome} exact>
        <OnboardingWelcome />
      </Route>

      <Route path={WalletRoutes.Onboarding} exact>
        <Redirect to={WalletRoutes.OnboardingWelcome} />
      </Route>

      <Redirect to={WalletRoutes.Onboarding} />

    </Switch>
  )
}

export default OnboardingRoutes
