// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useGetWalletsToImportQuery, //
} from '../../../../common/slices/api.slice'

// types
import { WalletRoutes } from '../../../../constants/types'

// components
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import {
  OnboardingStepsNavigation, //
} from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

// styles
import { WalletLink } from '../../../../components/shared/style'

import {
  StyledWrapper,
  Title,
  Description,
  MainWrapper,
} from '../onboarding.style'

import {
  BraveWalletIcon,
  CardButton,
  CardButtonTextContainer,
  LinkRow,
  MetaMaskIcon,
} from './import-or-restore-wallet.style'

export const OnboardingImportOrRestoreWallet = () => {
  // queries
  const { data: importableWallets } = useGetWalletsToImportQuery()

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>
          <OnboardingStepsNavigation preventSkipAhead />

          <div>
            <Title>
              {getLocale(S.BRAVE_WALLET_IMPORT_OR_RESTORE_WALLET_TITLE)}
            </Title>
            <Description>
              {getLocale(S.BRAVE_WALLET_IMPORT_OR_RESTORE_DESCRIPTION)}
            </Description>
          </div>

          <CardButton to={WalletRoutes.OnboardingRestoreWallet}>
            <CardButtonTextContainer>
              <p>{getLocale(S.BRAVE_WALLET_RESTORE_MY_BRAVE_WALLET)}</p>
              <p>
                {getLocale(S.BRAVE_WALLET_RESTORE_MY_BRAVE_WALLET_DESCRIPTION)}
              </p>
            </CardButtonTextContainer>
            <BraveWalletIcon />
          </CardButton>

          {importableWallets?.isMetaMaskInitialized && (
            <CardButton to={WalletRoutes.OnboardingImportMetaMask}>
              <CardButtonTextContainer>
                <p>{getLocale(S.BRAVE_WALLET_IMPORT_FROM_META_MASK)}</p>
                <p>
                  {getLocale(S.BRAVE_WALLET_IMPORT_FROM_META_MASK_DESCRIPTION)}
                </p>
              </CardButtonTextContainer>
              <MetaMaskIcon />
            </CardButton>
          )}

          <LinkRow>
            <WalletLink to={WalletRoutes.OnboardingNewWalletCreatePassword}>
              {getLocale(S.BRAVE_WALLET_CREATE_WALLET_INSTEAD_LINK)}
            </WalletLink>
          </LinkRow>
        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}
