// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/nala/tokens/css/variables'
import LeoButton from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'

// Assets
import BackgroundImage from './assets/onboarding_bg.svg'

export const OnboardingBackground = styled.div`
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
    background-image: url(${BackgroundImage});
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

export const Button = styled(LeoButton)`
  --leo-button-color: ${leo.color.black};
  @media (prefers-color-scheme: dark) {
    --leo-button-color: ${leo.color.container.highlight};
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
