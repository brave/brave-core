// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

// assets
import backgroundImageLight from '../../../../assets/svg-icons/onboarding/welcome_background_light.svg'
import backgroundImageDark from '../../../../assets/svg-icons/onboarding/welcome_background_dark.svg'
// styles
import { Column } from '../../../../components/shared/style'
import { layoutPanelWidth } from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const WelcomePageBackground = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-image: url(${backgroundImageLight});
  background-repeat: no-repeat;
  background-size: cover;
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 0;

  @media (prefers-color-scheme: dark) {
    background-image: url(${backgroundImageDark});
  }
`

export const WelcomePageWrapper = styled(Column)`
  display: flex;
  justify-content: center;
  align-items: center;
  min-width: 100%;
  padding-top: 48px;

  @media (min-width: 1025px) {
    padding-top: 158px;
  }
`

export const Content = styled(Column)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  z-index: 3;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0 24px;
  }

  @media screen and (min-width: 768px) and (max-width: 1024px) {
    width: 100%;
    padding-left: 24px;
    padding-right: 24px;
  }

  @media (min-width: 1025px) {
    width: 812px;
  }
`

export const BraveIcon = styled(Icon).attrs({
  name: 'brave-icon-release-color'
})`
  --leo-icon-size: 20px;
`

export const Title = styled.h4`
  font: ${leo.font.large.semibold};
  color: ${leo.color.text.primary};
  margin: 0;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    font-size: 18px;
  }
`

export const Heading = styled.h1`
  font: ${leo.font.heading.display1};
  color: ${leo.color.text.primary};
  margin: 0;
  padding-bottom: 16px;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    font-size: 36px;
    line-height: 48px;
  }
`

export const SubHeading = styled.h3`
  color: ${leo.color.text.tertiary};
  font-size: 22px;
  font-style: normal;
  font-weight: 400;
  line-height: 32px;
  margin: 0;
  padding: 0 0 48px 0;
`

export const ActionsContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  width: 100%;
  gap: ${leo.spacing['3Xl']};

  /* media query for desktop */
  @media (min-width: 1025px) {
    flex-direction: row;
  }
`

export const Footer = styled.footer`
  color: ${leo.color.text.secondary};
  font: ${leo.font.default.regular};
  margin: 0;
  padding-bottom: 72px;
`
