// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import WelcomeBackgroundSVG from './images/onboarding-welcome-background.svg'
import WelcomeIcon from '../../../../assets/svg-icons/onboarding/brave-wallet.svg'
import WelcomeIconDark from '../../../../assets/svg-icons/onboarding/brave-wallet-dark.svg'

export const OnboardingWelcomeWrapper = styled.div`
  width: 100%;
  height: 100%;
  padding: 80px;
  display: flex;
  flex-direction: column;
  align-items: center;
  overflow-x: hidden;
  background-repeat: no-repeat;
  background-image: url(${WelcomeBackgroundSVG});
  background-position: 50% 90%;
  background-size: 88%;
`

export const PageIcon = styled.div`
  width: 240px;
  height: 183px;
  background: url(${WelcomeIcon});
  margin-bottom: 28px;
  background-repeat: no-repeat;
  @media (prefers-color-scheme: dark) {
    background: url(${WelcomeIconDark});
  }
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 6px;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 400px;
  text-align: center;
  margin-bottom: 40px;
  letter-spacing: 0.01em;
`

export const LearnMoreLink = styled.a`
    font-family: Poppins;
    font-style: normal;
    font-weight: 600;
    font-size: 12px;
    line-height: 20px;
    text-decoration: none;
    text-align: center;
    color: ${(p) => p.theme.color.interactive05};
    display: flex;
    flex-direction: column;
    justify-content: center;
    height: 40px;
`

export const ButtonContainer = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  gap: 20px;
  & > * {
    width: 100%;
  }
`
