// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'
import Button from '@brave/leo/react/button'

// types
import { WalletRoutes } from '../../../../../constants/types'

// utils
import { getLocale } from '../../../../../../common/locale'

// styles
import { Row, VerticalSpace } from '../../../../../components/shared/style'
import {
  StyledWrapper,
  ContentWrapper,
  Content,
  BackButtonIcon,
  Title,
  Subtitle,
  WalletTitle,
  StaticBackground,
  BackgroundGradientBottomLayer,
  BraveIcon,
  HeaderWrapper,
  TitleSection,
  BackButtonWrapper,
} from './content_layout.style'

interface Props {
  title?: string | React.ReactNode
  subTitle?: string
  showBackButton?: boolean
  centerContent?: boolean
  padding?: string
  children: React.ReactNode
}

export const OnboardingContentLayout = ({
  title,
  subTitle,
  centerContent,
  showBackButton = true,
  padding,
  children,
}: Props) => {
  // routing
  const history = useHistory()
  const { pathname } = useLocation()

  // computed
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // methods
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
    <StyledWrapper>
      <StaticBackground />
      <BackgroundGradientBottomLayer />

      <TitleSection>
        <Row
          gap='10px'
          justifyContent='flex-start'
        >
          <BraveIcon />
          <WalletTitle>{getLocale('braveWalletTitle')}</WalletTitle>
        </Row>
      </TitleSection>
      <ContentWrapper>
        <BackButtonWrapper>
          {showBackButton ? (
            <Button
              kind='plain'
              size='small'
              onClick={handleBackButtonClick}
            >
              <BackButtonIcon />
            </Button>
          ) : (
            <VerticalSpace space='36px' />
          )}
        </BackButtonWrapper>
        <VerticalSpace space='7px' />
        <HeaderWrapper>
          {typeof title === 'string' ? <Title>{title}</Title> : title}

          {subTitle ? (
            <Row
              justifyContent='center'
              margin='16px 0 19px'
            >
              <Subtitle>{subTitle}</Subtitle>
            </Row>
          ) : null}
        </HeaderWrapper>
        <Content
          padding={padding}
          centerContent={centerContent}
        >
          {children}
        </Content>
      </ContentWrapper>
    </StyledWrapper>
  )
}
