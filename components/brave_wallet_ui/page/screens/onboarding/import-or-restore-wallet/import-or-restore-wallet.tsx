// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import { PageState, WalletRoutes } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'

// styles
import { WalletLink } from '../../../../components/shared/style'

import {
  StyledWrapper,
  Title,
  Description,
  MainWrapper
} from '../onboarding.style'

import {
  BraveWalletIcon,
 CardButton,
 CardButtonTextContainer,
 LegacyWalletIcon,
 LinkRow,
 MetaMaskIcon
} from './import-or-restore-wallet.style'

export const OnboardingImportOrRestoreWallet = () => {
  // redux
  const dispatch = useDispatch()
  const importWalletError = useSelector(({ page }: { page: PageState }) => page.importWalletError)
  const isImportWalletsCheckComplete = useSelector(({ page }: { page: PageState }) => page.isImportWalletsCheckComplete)
  const isLegacyCryptoWalletsInitialized = useSelector(({ page }: { page: PageState }) => page.isCryptoWalletsInitialized)

  // effects
  React.useEffect(() => {
    // reset any pending import errors
    if (importWalletError?.hasError) {
      dispatch(WalletPageActions.setImportWalletError({
        hasError: false
      }))
    }
  }, [importWalletError?.hasError])

  React.useEffect(() => {
    if (!isImportWalletsCheckComplete) {
      // check if MM or legacy wallet is installed
      dispatch(WalletPageActions.checkWalletsToImport())
    }
  }, [isImportWalletsCheckComplete])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <StepsNavigation
            goBackUrl={WalletRoutes.Onboarding}
            currentStep={''}
            steps={[]}
          />

          <div>
            <Title>
              {getLocale('braveWalletImportOrRestoreWalletTitle')}
            </Title>
            <Description>
              {getLocale('braveWalletImportOrRestoreDescription')}
            </Description>
          </div>

          <CardButton
            to={WalletRoutes.OnboardingRestoreWallet}
          >
            <CardButtonTextContainer>
              <p>
                {getLocale('braveWalletRestoreMyBraveWallet')}
              </p>
              <p>
                {getLocale('braveWalletRestoreMyBraveWalletDescription')}
              </p>
            </CardButtonTextContainer>
            <BraveWalletIcon />
          </CardButton>

          <CardButton
            to={WalletRoutes.OnboardingImportMetaMask}
          >
            <CardButtonTextContainer>
              <p>
                {getLocale('braveWalletImportFromMetaMask')}
              </p>
              <p>
                {getLocale('braveWalletImportFromMetaMaskDescription')}
              </p>
            </CardButtonTextContainer>
            <MetaMaskIcon />
          </CardButton>

          {isLegacyCryptoWalletsInitialized &&
            <CardButton
              to={WalletRoutes.OnboardingImportMetaMask}
            >
              <CardButtonTextContainer>
                <p>
                  {getLocale('braveWalletImportFromLegacy')}
                </p>
              </CardButtonTextContainer>
              <LegacyWalletIcon />
            </CardButton>
          }

          <LinkRow>
            <WalletLink
              to={WalletRoutes.OnboardingCreatePassword}
            >
              {getLocale('braveWalletCreateWalletInsteadLink')}
            </WalletLink>
          </LinkRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingImportOrRestoreWallet
