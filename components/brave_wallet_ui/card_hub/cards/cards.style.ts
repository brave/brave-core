// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

export const StyledCard = styled.div`
  --card-height: 220px;
  --card-thickness: 2px;
  --card-lift: 0px;
  --tilt-x: 0deg;
  --tilt-y: 0deg;
  --rotate-z: 0deg;
  --shadow-x: 0px;
  --shadow-y: 4px;
  cursor: pointer;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  height: var(--card-height);
  --card-lip: color-mix(in srgb, ${leo.color.divider.subtle} 45%, transparent);
  border-radius: ${leo.radius.xxl};
  box-shadow:
    0 -1px 0 0 var(--card-lip),
    ${leo.effect.elevation['01']},
    var(--shadow-x) var(--shadow-y) 16px ${leo.color.elevation.secondary};
  position: relative;
  transform-origin: center center;
  transform-style: preserve-3d;
  transform: translateY(var(--card-lift)) rotateX(var(--tilt-x))
    rotateY(var(--tilt-y)) rotateZ(var(--rotate-z));
  transition:
    box-shadow ${leo.duration.m} ${leo.easing.out},
    transform ${leo.duration.m} ${leo.easing.out};

  &::before {
    content: '';
    position: absolute;
    inset: 0;
    border-radius: inherit;
    background-color: color-mix(
      in srgb,
      ${leo.color.divider.strong} 45%,
      transparent
    );
    transform: translateZ(calc(-1 * var(--card-thickness)));
    pointer-events: none;
  }

  &:hover {
    box-shadow:
      0 -1px 0 0 var(--card-lip),
      ${leo.effect.elevation['04']},
      var(--shadow-x) var(--shadow-y) 28px ${leo.color.elevation.secondary};
  }

  @media (prefers-color-scheme: dark) {
    --card-lip: color-mix(in srgb, ${leo.color.white} 24%, transparent);

    &::before {
      background-color: color-mix(in srgb, ${leo.color.white} 16%, transparent);
    }
  }

  @media (prefers-reduced-motion: reduce) {
    transform: translateY(var(--card-lift));
  }
`

export const CardFace = styled.div`
  position: relative;
  z-index: 1;
  width: 100%;
  height: 100%;
  border-radius: inherit;
  overflow: hidden;
  background-color: ${leo.color.container.background};
  transform: translateZ(0);

  &::after {
    content: '';
    position: absolute;
    inset: 0;
    border-radius: inherit;
    box-shadow: inset 0 1px 0 0 var(--card-lip);
    pointer-events: none;
  }
`

export const CardBackground = styled.div`
  width: 100%;
  height: 100%;
  position: absolute;
  inset: 0;
  z-index: -1;
  border-radius: inherit;
`
