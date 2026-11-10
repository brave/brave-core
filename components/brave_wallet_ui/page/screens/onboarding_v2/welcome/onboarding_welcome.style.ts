// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

// Assets
import OnboardingBackground from '../assets/onboarding_bg.svg'

// Shared Styles
import { Column, Text } from '$wallet/components/shared/style'
import { layoutPanelWidth } from '$wallet/page/components/wallet_page_wrapper/wallet_page_wrapper.style'

export const WelcomePageBackground = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${leo.color.page.background};
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 0;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    inset: -8%;
    background-image: url(${OnboardingBackground});
    background-repeat: no-repeat;
    background-size: cover;
    background-position: center;
    opacity: 0.08;
    pointer-events: none;
    animation: onboarding-bg-drift 56s ease-in-out infinite;

    @media (prefers-color-scheme: dark) {
      filter: invert(1);
    }

    @media (prefers-reduced-motion: reduce) {
      animation: none;
    }
  }

  @keyframes onboarding-bg-drift {
    0%,
    100% {
      transform: translate3d(0, 0, 0) scale(1);
    }
    40% {
      transform: translate3d(-2.2%, 1.4%, 0) scale(1.035);
    }
    70% {
      transform: translate3d(1.6%, -1.1%, 0) scale(1.02);
    }
  }
`

export const WelcomePageWrapper = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 100%;
  height: 100vh;
`

export const Content = styled(Column)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  z-index: 3;
  max-width: 812px;

  @media screen and (min-width: 768px) and (max-width: 1024px) {
    width: 100%;
  }
`

export const BraveIcon = styled(Icon).attrs({
  name: 'brave-icon-release-color',
})`
  --leo-icon-size: 48px;
`

export const Heading = styled.h2`
  font: ${leo.font.heading.h1};
  color: ${leo.color.text.primary};
  margin: 0;
`

export const Title = styled(Text)`
  max-width: 280px;
`

export const Description = styled(Text)`
  max-width: 640px;
`

export const ActionsContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  width: 100%;
  gap: ${leo.spacing.xl};
  padding-bottom: ${leo.spacing['3Xl']};
  @media (min-width: ${layoutPanelWidth}px) {
    flex-direction: row;
  }
`
