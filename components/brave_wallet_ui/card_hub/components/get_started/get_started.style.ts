// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { keyframes } from 'styled-components'
import * as nala from '@brave/nala/tokens/css/variables'
import NalaButton from '@brave/nala/react/button'
import NalaIcon from '@brave/nala/react/icon'

// Assets
import CardGraphic from '$wallet/card_hub/cards/rewards_card/rewards_card_background.jpg'
import CryptoCardGraphic from '$wallet/card_hub/cards/crypto/crypto_card_background.png'

// Shared Styles
import { Column } from '$wallet/components/shared/style'

export const Screen = styled.div`
  position: relative;
  overflow: hidden;
  width: 100%;
  height: 100%;
`

export const Glow = styled.div<{ $corner: 'topLeft' | 'bottomRight' }>`
  position: absolute;
  width: 260px;
  height: 260px;
  border-radius: ${nala.radius.full};
  pointer-events: none;
  filter: blur(64px);
  opacity: 0.4;
  background: ${(p) =>
    p.$corner === 'topLeft'
      ? `radial-gradient(circle, color-mix(in srgb, ${nala.color.primitive.brands.rorange2} 33%, transparent), transparent 70%)`
      : `radial-gradient(circle, color-mix(in srgb, ${nala.color.primitive.purple['60']} 33%, transparent), transparent 70%)`};
  ${(p) =>
    p.$corner === 'topLeft'
      ? 'top: -96px; left: -64px;'
      : 'bottom: -80px; right: -64px;'}
`

export const Body = styled(Column)`
  position: relative;
  z-index: 1;
`

export const Button = styled(NalaButton)`
  --leo-button-color: ${nala.color.black};
  @media (prefers-color-scheme: dark) {
    --leo-button-color: ${nala.color.container.highlight};
  }
`

const float = keyframes`
  0%,
  100% {
    translate: 0 0;
  }
  50% {
    translate: 0 -6px;
  }
`

const cardFloat = keyframes`
  0%,
  100% {
    translate: 0 0;
  }
  50% {
    translate: 0 -3px;
  }
`

export const Hero = styled.div`
  position: relative;
  width: 100%;
  height: 236px;
`

export const BackingCard = styled.div<{
  $x: number
  $y: number
  $rotate: number
  $zIndex: number
  $delay: number
  $graphic?: 'crypto'
}>`
  position: absolute;
  left: 50%;
  top: ${(p) => 36 + p.$y}px;
  z-index: ${(p) => p.$zIndex};
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  align-items: flex-start;
  width: 214px;
  height: 132px;
  margin-left: ${(p) => -107 + p.$x}px;
  border-radius: ${nala.radius.l};
  border: 1px solid ${nala.color.divider.subtle};
  background-color: ${nala.color.container.background};
  background-image: ${(p) =>
    p.$graphic === 'crypto' ? `url(${CryptoCardGraphic})` : 'none'};
  background-repeat: no-repeat;
  background-size: cover;
  background-position: center;
  box-shadow: ${nala.effect.elevation['02']};
  transform: rotate(${(p) => p.$rotate}deg);
  padding: ${nala.spacing.l};
  animation: ${cardFloat} 5.5s ease-in-out ${(p) => p.$delay}s infinite;

  @media (prefers-reduced-motion: reduce) {
    animation: none;
  }
`

export const Card = styled.div`
  position: absolute;
  left: 50%;
  top: 36px;
  z-index: 3;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  width: 214px;
  height: 132px;
  margin-left: -107px;
  padding: ${nala.spacing.l};
  border-radius: ${nala.radius.l};
  overflow: hidden;
  background-color: #ff1c00;
  background-image: url(${CardGraphic});
  background-repeat: no-repeat;
  background-position: center;
  /* The JPEG's bottom row is a pale fringe. Cover pins that row to the
     card edge, so scale past 100% and let overflow clip it. */
  background-size: 104% 104%;
  box-shadow: ${nala.effect.elevation['03']};
  transform: rotate(-2deg);
  color: ${nala.color.white};
  animation: ${cardFloat} 5.5s ease-in-out infinite;

  @media (prefers-reduced-motion: reduce) {
    animation: none;
  }
`

export const Floater = styled.div<{
  $delay: number
  $zIndex?: number
  $top?: string
  $left?: string
  $right?: string
  $bottom?: string
}>`
  position: absolute;
  z-index: ${(p) => p.$zIndex ?? 4};
  top: ${(p) => p.$top ?? 'auto'};
  left: ${(p) => p.$left ?? 'auto'};
  right: ${(p) => p.$right ?? 'auto'};
  bottom: ${(p) => p.$bottom ?? 'auto'};
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.12));
  animation: ${float} 4.5s ease-in-out ${(p) => p.$delay}s infinite;

  @media (prefers-reduced-motion: reduce) {
    animation: none;
  }
`

export const BatIcon = styled(NalaIcon)`
  --leo-icon-size: 40px;
`

export const NetworkIcon = styled.img<{ $size?: number }>`
  width: ${(p) => p.$size ?? 30}px;
  height: ${(p) => p.$size ?? 30}px;
`

export const Tile = styled.div`
  --leo-icon-size: 18px;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 26px;
  height: 26px;
  border-radius: ${nala.radius.m};
  background-color: ${nala.color.white};
`

export const CardLogo = styled.img<{
  $height?: number
  $invertOnDark?: boolean
}>`
  height: ${(p) => p.$height ?? 18}px;
  width: auto;
  flex-shrink: 0;

  @media (prefers-color-scheme: dark) {
    filter: ${(p) => (p.$invertOnDark ? 'brightness(0) invert(1)' : 'none')};
  }
`
