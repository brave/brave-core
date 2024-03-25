// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Button from '@brave/leo/react/button'

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
  BackButtonWrapper
} from './onboarding-content-layout.style'

interface Props {
  title?: string | React.ReactNode
  subTitle?: string
  showBackButton?: boolean
  centerContent?: boolean
  children: React.ReactNode
}

export const OnboardingContentLayout = ({
  title,
  subTitle,
  centerContent,
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
        <BackButtonWrapper>
          {showBackButton ? (
            <Button
              kind='plain'
              size='small'
              onClick={() => history.goBack()}
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
            <>
              <VerticalSpace space='16px' />
              <Subtitle>{subTitle}</Subtitle>
              <VerticalSpace space='19px' />
            </>
          ) : null}
        </HeaderWrapper>
        <Content centerContent={centerContent}>{children}</Content>
      </ContentWrapper>
    </StyledWrapper>
  )
}
