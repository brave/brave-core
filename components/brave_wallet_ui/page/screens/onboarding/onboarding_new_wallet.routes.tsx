// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Switch, useHistory } from 'react-router'

// types
import { WalletRoutes } from '../../../constants/types'

// selectors
import {
  useSafePageSelector,
  useSafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'
import { PageSelectors } from '../../selectors'

// components
import { OnboardingDisclosures } from './disclosures/disclosures'
import {
  BackupRecoveryPhrase //
} from '../backup-wallet/backup-recovery-phrase/backup-recovery-phrase'
import {
  OnboardingCreatePassword //
} from './create-password/onboarding-create-password'
import {
  RecoveryPhraseExplainer //
} from '../backup-wallet/explain-recovery-phrase/explain-recovery-phrase'
import {
  VerifyRecoveryPhrase //
} from '../backup-wallet/verify-recovery-phrase/verify-recovery-phrase-v3'
import {
  ProtectedRoute //
} from '../../../components/shared/protected-routing/protected-route'
import {
  OnboardingNetworkSelection //
} from './network_selection/onboarding_network_selection'

export const OnboardingNewWalletRoutes = () => {
  // routing
  const history = useHistory()

  // redux
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const termsAcknowledged = useSafePageSelector(
    PageSelectors.walletTermsAcknowledged
  )
  const setupStillInProgress = useSafePageSelector(
    PageSelectors.setupStillInProgress
  )

  // render
  return (
    <Switch>
      <ProtectedRoute
        path={WalletRoutes.OnboardingNewWalletTerms}
        exact
        requirement={!isWalletCreated}
        redirectRoute={WalletRoutes.OnboardingComplete}
      >
        <OnboardingDisclosures />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingNewWalletNetworkSelection}
        exact
        requirement={termsAcknowledged && !isWalletCreated}
        redirectRoute={WalletRoutes.OnboardingNewWalletTerms}
      >
        <OnboardingNetworkSelection />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingNewWalletCreatePassword}
        exact
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingNewWalletTerms}
      >
        <OnboardingCreatePassword
          onWalletCreated={() =>
            history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
          }
        />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingExplainRecoveryPhrase}
        exact
        requirement={isWalletCreated && setupStillInProgress}
        redirectRoute={WalletRoutes.OnboardingWelcome}
      >
        <RecoveryPhraseExplainer />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingBackupRecoveryPhrase}
        exact
        requirement={isWalletCreated && setupStillInProgress}
        redirectRoute={WalletRoutes.OnboardingWelcome}
      >
        <BackupRecoveryPhrase />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingVerifyRecoveryPhrase}
        exact
        requirement={isWalletCreated && setupStillInProgress}
        redirectRoute={WalletRoutes.OnboardingWelcome}
      >
        <VerifyRecoveryPhrase />
      </ProtectedRoute>

      <Redirect
        to={
          isWalletCreated
            ? WalletRoutes.OnboardingComplete
            : WalletRoutes.OnboardingWelcome
        }
      />
    </Switch>
  )
}
