// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  Redirect,
  Route,
  Switch
} from 'react-router'
import { useSafePageSelector } from '../../../common/hooks/use-safe-selector'

// types
import { WalletRoutes } from '../../../constants/types'
import { PageSelectors } from '../../selectors'

// components
import { BackupEnterPassword } from './backup-enter-password/backup-enter-password'
import { BackupRecoveryPhrase } from './backup-recovery-phrase/backup-recovery-phrase'
import { RecoveryPhraseExplainer } from './explain-recovery-phrase/explain-recovery-phrase'
import { VerifyRecoveryPhrase } from './verify-recovery-phrase/verify-recovery-phrase'

export const WALLET_BACKUP_STEPS = [
  WalletRoutes.Backup,
  WalletRoutes.BackupExplainRecoveryPhrase,
  WalletRoutes.BackupRecoveryPhrase,
  WalletRoutes.BackupVerifyRecoveryPhrase
]

export const BackupWalletRoutes = () => {
  // redux
  const hasMnemonic = useSafePageSelector(PageSelectors.hasMnemonic)

  // render
  return (
    <Switch>

      <Route path={WalletRoutes.Backup} exact>
        <BackupEnterPassword />
      </Route>

      {hasMnemonic && <Route path={WalletRoutes.BackupExplainRecoveryPhrase} exact>
        <RecoveryPhraseExplainer />
      </Route>}

      {hasMnemonic && <Route path={WalletRoutes.BackupRecoveryPhrase} exact>
        <BackupRecoveryPhrase />
      </Route>}

      {hasMnemonic && <Route path={WalletRoutes.BackupVerifyRecoveryPhrase} exact>
        <VerifyRecoveryPhrase />
      </Route>}

      <Redirect to={WalletRoutes.Backup} />

    </Switch>
  )
}

export default BackupWalletRoutes
