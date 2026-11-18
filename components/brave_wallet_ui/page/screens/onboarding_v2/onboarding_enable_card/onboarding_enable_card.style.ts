// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/nala/tokens/css/variables'

// Assets
import CardGraphic from '$wallet/card_hub/cards/rewards_card/rewards_card_background.jpg'

// Shared Styles
import { Button } from '../onboarding.style'
import { Text } from '$wallet/components/shared/style'

export const Card = styled.div`
  position: relative;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  width: 300px;
  height: 189px;
  margin: ${leo.spacing['2Xl']} 0;
  padding: ${leo.spacing.xl};
  border-radius: ${leo.radius.l};
  overflow: hidden;
  background-color: #ff1c00;
  background-image: url(${CardGraphic});
  background-repeat: no-repeat;
  background-position: center;
  /* The JPEG's bottom row is a pale fringe. Cover pins that row to the
     card edge, so scale past 100% and let overflow clip it. */
  background-size: 104% 104%;
  box-shadow: ${leo.effect.elevation['03']};
  transform: rotate(-2deg);
  color: ${leo.color.white};
`

export const BraveCardTextBubble = styled(Text)`
  display: inline-flex;
  align-items: center;
  color: ${leo.color.primitive.brands.rorange2};
  padding: 4px 17px 4px;
  line-height: 1.15;
  border-radius: ${leo.radius.full};
  background-color: color-mix(
    in srgb,
    ${leo.color.primitive.brands.rorange2} 10%,
    transparent
  );
`

export const PrintInCharacters = styled.span`
  display: inline-flex;
`

export const PrintInCharacter = styled.span`
  display: inline-block;
  white-space: pre;
  animation:
    onboarding-print-in-rise 0.42s cubic-bezier(0.16, 1, 0.3, 1) both,
    onboarding-print-in-fade 0.21s linear both;

  @media (prefers-reduced-motion: reduce) {
    animation: none;
  }

  @keyframes onboarding-print-in-rise {
    from {
      transform: translateY(0.32em);
    }
    to {
      transform: translateY(0);
    }
  }

  @keyframes onboarding-print-in-fade {
    from {
      opacity: 0;
    }
    to {
      opacity: 1;
    }
  }
`

export const Description = styled(Text)`
  max-width: 440px;
`

export const Tile = styled.div`
  --leo-icon-size: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 38px;
  height: 38px;
  border-radius: ${leo.radius.m};
  background-color: ${leo.color.white};
`

export const CardDetails = styled(Text)`
  opacity: 0.8;
`

export const MastercardLogo = styled.img`
  height: 24px;
  width: auto;
  flex-shrink: 0;
`

export const BenefitIconBubble = styled.div`
  --leo-icon-size: 14px;
  --leo-icon-color: ${leo.color.primitive.brands.rorange2};
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border-radius: ${leo.radius.full};
  background-color: color-mix(
    in srgb,
    ${leo.color.primitive.brands.rorange2} 10%,
    transparent
  );
`

export const NotNowButton = styled(Button)`
  --leo-button-color: ${leo.color.text.secondary};
`
