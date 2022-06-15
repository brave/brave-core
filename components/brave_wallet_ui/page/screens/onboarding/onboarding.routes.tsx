// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  Redirect,
  Route,
  Switch
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
import { OnboardingDisclosures } from './disclosures/disclosures'

// types
import { WalletRoutes } from '../../../constants/types'

export const OnboardingRoutes = () => {
  // render
  return (
    <Switch>

      <Route path={WalletRoutes.OnboardingDisclosures} exact>
        <OnboardingDisclosures />
      </Route>

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
          restoreFrom={'metamask'}
        />
      </Route>

      <Route path={WalletRoutes.OnboardingImportCryptoWallets} exact>
        <OnboardingRestoreFromRecoveryPhrase
          restoreFrom='legacy'
        />
      </Route>

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
