// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import {
  Redirect,
  Route,
  Switch,
  useHistory
} from 'react-router'

// components
import { BackupRecoveryPhrase } from '../backup-wallet/backup-recovery-phrase/backup-recovery-phrase'
import { OnboardingCreatePassword } from './create-password/onboarding-create-password'
import { RecoveryPhraseExplainer } from '../backup-wallet/explain-recovery-phrase/explain-recovery-phrase'
import { VerifyRecoveryPhrase } from '../backup-wallet/verify-recovery-phrase/verify-recovery-phrase'
import { OnboardingWelcome } from './welcome/onboarding-welcome'
import { OnboardingImportOrRestoreWallet } from './import-or-restore-wallet/import-or-restore-wallet'
import { OnboardingRestoreFromRecoveryPhrase } from './restore-from-recovery-phrase/restore-from-recovery-phrase'

// types
import { PageState, WalletRoutes, WalletState } from '../../../constants/types'
import { OnboardingSuccess } from './onboarding-success/onboarding-success'
import { OnboardingConnectHardwareWallet } from './connect-hardware/onboarding-connect-hardware-wallet'

export const OnboardingRoutes = () => {
  // routing
  const history = useHistory()

  // redux
  const isWalletCreated = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isWalletCreated)
  const termsAcknowledged = useSelector(({ page }: { page: PageState }) => page.walletTermsAcknowledged)

  // methods
  const goToConnectHardware = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingConnectHardwareWalletStart)
  }, [])

  const goToExplainRecoveryPhrase = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
  }, [])

  // render
  return (
    <Switch>

      {!isWalletCreated &&
        <Route path={WalletRoutes.OnboardingWelcome} exact>
          <OnboardingWelcome />
        </Route>
      }

      {!isWalletCreated &&
        <Route path={WalletRoutes.Onboarding} exact>
          <Redirect to={WalletRoutes.OnboardingWelcome} />
        </Route>
      }

      {(!termsAcknowledged && !isWalletCreated) &&
        <Redirect to={WalletRoutes.OnboardingWelcome} />
      }
      
      <Route path={WalletRoutes.OnboardingCreatePassword} exact>
        <OnboardingCreatePassword
          onWalletCreated={goToExplainRecoveryPhrase}
        />
      </Route>

      <Route path={WalletRoutes.OnboardingConnectHarwareWalletCreatePassword} exact>
        <OnboardingCreatePassword
          isHardwareOnboarding={true}
          onWalletCreated={goToConnectHardware}
        />
      </Route>

      
      {!isWalletCreated &&
        <Route path={WalletRoutes.OnboardingImportOrRestore} exact>
          <OnboardingImportOrRestoreWallet />
        </Route>
      }

      {!isWalletCreated &&
        <Route path={WalletRoutes.OnboardingRestoreWallet} exact>
          <OnboardingRestoreFromRecoveryPhrase
            key='seed' // keys are set here to prevent holding state between page changes
            restoreFrom='seed'
          />
        </Route>
      }

      {!isWalletCreated &&
        <Route path={WalletRoutes.OnboardingImportMetaMask} exact>
          <OnboardingRestoreFromRecoveryPhrase
            key='metamask'
            restoreFrom='metamask'
          />
        </Route>
      }

      {!isWalletCreated &&
        <Route path={WalletRoutes.OnboardingImportMetaMaskSeed} exact>
          <OnboardingRestoreFromRecoveryPhrase
            key='metamask-seed'
            restoreFrom='metamask-seed'
          />
        </Route>
      }

      {!isWalletCreated &&
        <Route path={WalletRoutes.OnboardingImportCryptoWallets} exact>
          <OnboardingRestoreFromRecoveryPhrase
            key='legacy'
            restoreFrom='legacy'
          />
        </Route>
      }

      {!isWalletCreated &&
        <Route path={WalletRoutes.OnboardingImportCryptoWalletsSeed} exact>
          <OnboardingRestoreFromRecoveryPhrase
            key='legacy-seed'
            restoreFrom='legacy-seed'
          />
        </Route>
      }

      <Route path={WalletRoutes.OnboardingConnectHardwareWalletStart} exact>
        <OnboardingConnectHardwareWallet />
      </Route>

      <Route path={WalletRoutes.OnboardingConnectHardwareWallet} exact>
        <OnboardingConnectHardwareWallet />
      </Route>

      {!isWalletCreated && <Redirect to={WalletRoutes.OnboardingWelcome} />}

      <Route path={WalletRoutes.OnboardingExplainRecoveryPhrase} exact>
        <RecoveryPhraseExplainer />
      </Route>

      <Route path={WalletRoutes.OnboardingBackupRecoveryPhrase} exact>
        <BackupRecoveryPhrase />
      </Route>

      <Route path={WalletRoutes.OnboardingVerifyRecoveryPhrase} exact>
        <VerifyRecoveryPhrase />
      </Route>

      <Route path={WalletRoutes.OnboardingComplete} exact>
        <OnboardingSuccess />
      </Route>

      <Redirect to={WalletRoutes.OnboardingComplete} />

    </Switch>
  )
}

export default OnboardingRoutes
