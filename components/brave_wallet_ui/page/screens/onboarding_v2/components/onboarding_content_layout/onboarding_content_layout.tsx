// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'
import Icon from '@brave/nala/react/icon'

// Types
import { WalletRoutes } from '$wallet/constants/types'

// Utils
import { getLocale } from '$web-common/locale'

// Styled Components
import { Row } from '$wallet/components/shared/style'
import {
  StyledWrapper,
  ContentWrapper,
  BackButton,
} from './onboarding_content_layout.style'
import {
  OnboardingBackground,
  BraveIcon,
  Heading,
} from '$wallet/page/screens/onboarding_v2/onboarding.style'

interface Props {
  showBackButton?: boolean
  children: React.ReactNode
}

export const OnboardingContentLayout = ({
  showBackButton = true,
  children,
}: Props) => {
  // Routing
  const history = useHistory()
  const { pathname } = useLocation()

  // Computed
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // Methods
  const handleBackButtonClick = React.useCallback(() => {
    // Handle onboarding back button
    if (
      isOnboarding
      && pathname === WalletRoutes.OnboardingBackupRecoveryPhrase
    ) {
      history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
      return
    }
    // Handle backup wallet back button
    if (pathname === WalletRoutes.Backup) {
      history.push(WalletRoutes.Portfolio)
      return
    }
    if (pathname === WalletRoutes.BackupExplainRecoveryPhrase) {
      history.push(WalletRoutes.Backup)
      return
    }
    if (pathname === WalletRoutes.BackupRecoveryPhrase) {
      history.push(WalletRoutes.BackupExplainRecoveryPhrase)
      return
    }
    history.goBack()
  }, [history, pathname, isOnboarding])

  return (
    <>
      <OnboardingBackground />
      <StyledWrapper>
        <Row
          justifyContent='flex-start'
          maxWidth='960px'
        >
          <BraveIcon />
          <Heading>{getLocale(S.BRAVE_WALLET_TITLE)}</Heading>
        </Row>
        <ContentWrapper>
          {showBackButton && (
            <Row
              justifyContent='flex-start'
              margin='0 0 12px 0'
            >
              <BackButton
                fab
                onClick={handleBackButtonClick}
                size='small'
                kind='outline'
              >
                <Icon name='arrow-left' />
              </BackButton>
            </Row>
          )}
          {children}
        </ContentWrapper>
      </StyledWrapper>
    </>
  )
}
