// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/nala/tokens/css/variables'

// Shared Styles
import { Text } from '$wallet/components/shared/style'

export const Description = styled(Text)`
  max-width: 440px;
`

export const CelebrationSlot = styled.div`
  position: relative;
  width: 56px;
  height: 56px;
  margin: 0px 0px 24px 0px;
`

export const CelebrationIcon = styled.div`
  position: relative;
  z-index: 1;
  display: flex;
  justify-content: center;
  align-items: center;
  width: 56px;
  height: 56px;
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.full};
  font-size: 30px;
  animation: onboarding-celebration-pop 0.6s linear both;

  @media (prefers-reduced-motion: reduce) {
    animation: none;
  }

  /* Spring approximation of stiffness 220, damping 18: a small overshoot
     that settles back to full size. */
  @keyframes onboarding-celebration-pop {
    0% {
      opacity: 0;
      scale: 0.6;
    }
    45% {
      opacity: 1;
      scale: 1.04;
    }
    70% {
      opacity: 1;
      scale: 0.99;
    }
    100% {
      opacity: 1;
      scale: 1;
    }
  }
`

export const ConfettiLayer = styled.div`
  position: absolute;
  inset: 0;
  overflow: visible;
  pointer-events: none;
`

export const ConfettiPiece = styled.span<{
  $x: number
  $y: number
  $rotate: number
  $delay: number
  $duration: number
  $round: boolean
  $color: string
}>`
  position: absolute;
  left: 50%;
  top: 50%;
  display: block;
  width: 6px;
  height: ${(p) => (p.$round ? '6px' : '10px')};
  border-radius: ${(p) => (p.$round ? '999px' : '1px')};
  background-color: ${(p) => p.$color};
  --confetti-x: ${(p) => p.$x}px;
  --confetti-y: ${(p) => p.$y}px;
  --confetti-rotate: ${(p) => p.$rotate}deg;
  animation:
    onboarding-confetti-move ${(p) => p.$duration}s
      cubic-bezier(0.16, 0.84, 0.44, 1) ${(p) => p.$delay}s both,
    onboarding-confetti-pop ${(p) => p.$duration}s linear ${(p) => p.$delay}s
      both;

  @media (prefers-reduced-motion: reduce) {
    animation: none;
    opacity: 0;
  }

  @keyframes onboarding-confetti-move {
    from {
      translate: -50% -50%;
      rotate: 0deg;
    }
    to {
      translate: calc(-50% + var(--confetti-x)) calc(-50% + var(--confetti-y));
      rotate: var(--confetti-rotate);
    }
  }

  @keyframes onboarding-confetti-pop {
    0% {
      opacity: 0;
      scale: 0.4;
    }
    15% {
      opacity: 1;
      scale: 1;
    }
    70% {
      opacity: 1;
      scale: 1;
    }
    100% {
      opacity: 0;
      scale: 0.9;
    }
  }
`
