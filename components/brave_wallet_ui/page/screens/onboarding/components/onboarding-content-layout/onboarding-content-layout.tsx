// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../../../common/locale'

// styles
import { Row, VerticalSpace } from '../../../../../components/shared/style'
import {
  StyledWrapper,
  ContentWrapper,
  Content,
  BackButton,
  BackButtonIcon,
  Title,
  Subtitle,
  WalletTitle,
  StaticBackground,
  BackgroundGradientBottomLayer,
  BraveIcon,
  HeaderWrapper,
  TitleSection
} from './onboarding-content-layout.style'

interface Props {
  title?: string | React.ReactNode
  subTitle?: string
  showBackButton?: boolean
  children: React.ReactNode
}

export const OnboardingContentLayout = ({
  title,
  subTitle,
  showBackButton = true,
  children
}: Props) => {
  // routing
  const history = useHistory()

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
        <Row justifyContent='flex-start'>
          {showBackButton ? (
            <BackButton onClick={() => history.goBack()}>
              <BackButtonIcon />
            </BackButton>
          ) : (
            <VerticalSpace space='36px' />
          )}
        </Row>
        <VerticalSpace space='7px' />
        <HeaderWrapper>
          {typeof title === 'string' ? <Title>{title}</Title> : title}

          {subTitle ? (
            <>
              <VerticalSpace space='16px' />
              <Subtitle>{subTitle}</Subtitle>
              <VerticalSpace space='19px' />
            </>
          ) : null}
        </HeaderWrapper>
        <Content>{children}</Content>
      </ContentWrapper>
    </StyledWrapper>
  )
}
