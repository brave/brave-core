// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Prompt, Route, Switch, useHistory } from 'react-router'

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
import { FundWalletScreen } from '../screens/fund-wallet/fund_wallet_v1'
import { SimplePageWrapper } from '../screens/page-screen.styles'
import {
  OnboardingSuccess //
} from '../screens/onboarding/onboarding_success/onboarding_success'
import { PartnersConsentModal } from '../../components/desktop/popup-modals/partners_consent_modal/partners_consent_modal'
import { Location } from 'history'
import { useLocalStorage } from '../../common/hooks/use_local_storage'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'

export const UnlockedWalletRoutes = ({
  sessionRoute
}: {
  sessionRoute: WalletRoutes | undefined
}) => {
  // state
  const [isModalOpen, setModalOpen] = React.useState(false)
  const [nextLocation, setNextLocation] = React.useState<Location | null>(null)
  const [shouldBlock, setShouldBlock] = React.useState(true)

  // hooks
  const history = useHistory()
  const [acceptedTerms, setAcceptedTerms] = useLocalStorage(
    LOCAL_STORAGE_KEYS.HAS_ACCEPTED_PARTNER_TERMS,
    false
  )

  const handleAccept = () => {
    setAcceptedTerms(true)
    setModalOpen(false)
    setShouldBlock(false)
    if (nextLocation) {
      history.push(nextLocation.pathname)
    }
  }

  const handleDecline = () => {
    setModalOpen(false)
    setNextLocation(null)
  }

  const handleBlockedNavigation = (location: Location) => {
    if (
      !acceptedTerms &&
      location.pathname.startsWith(WalletRoutes.FundWalletPageStart)
    ) {
      setModalOpen(true)
      setNextLocation(location)
      return false
    }
    return true
  }

  // render
  return (
    <>
      <Prompt
        when={shouldBlock}
        message={(location) => handleBlockedNavigation(location)}
      />
      <PartnersConsentModal
        isOpen={isModalOpen}
        onClose={() => {}}
        onCancel={handleDecline}
        onContinue={handleAccept}
      />
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
    </>
  )
}
