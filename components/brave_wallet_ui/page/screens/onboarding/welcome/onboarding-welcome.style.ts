// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css'

// assets
import backgroundImageLight from '../../../../assets/svg-icons/onboarding/welcome-background-light.svg'
import backgroundImageDark from '../../../../assets/svg-icons/onboarding/welcome-background-dark.svg'
// styles
import { Column, WalletButton } from '../../../../components/shared/style'

export const WelcomePageBackground = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-image: url(${backgroundImageLight});
  background-repeat: no-repeat;
  background-size: cover;
  position: absolute;
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

  @media (min-width: 768px) and (max-width: 1024px) {
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
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 600;
  line-height: 26px;
  color: ${leo.color.text.primary};
  margin: 0;
`

export const Heading = styled.h1`
  font-family: Poppins;
  font-size: 52px;
  font-style: normal;
  font-weight: 500;
  line-height: 68px;
  color: ${leo.color.text.primary};
  margin: 0;
`

export const SubHeading = styled.h4`
  font-family: Poppins;
  font-size: 22px;
  font-style: normal;
  font-weight: 400;
  line-height: 32px;
  color: ${leo.color.text.secondary};
`

export const ActionsContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  width: 100%;
  gap: ${leo.spacing['3Xl']};

  // media query for desktop
  @media (min-width: 1025px) {
    flex-direction: row;
  }
`

export const WatchOnlyWalletButton = styled(WalletButton)`
  color: ${leo.color.text.interactive};
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 600;
  line-height: 22px;
  padding: ${leo.spacing.l};
  width: 100%;
  text-align: center;
  border: none;
  background-color: transparent;
  cursor: pointer;
`

export const Footer = styled.footer`
  color: ${leo.color.text.secondary};
  font-family: 'Inter Variable';
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 22px;
  margin: 0;
`
