// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled, { css, keyframes } from 'styled-components'
import { Size, Brand } from './index'

interface ToggleProps {
  isOn: boolean
  size?: Size
  brand?: Brand
}

const moveBg = keyframes`
  0% {
    background-position: 0% 50%;
  }
  50% {
    background-position: 100% 50%;
  }
  100% {
    background-position: 0% 50%;
  }
`

export const ToggleBox = styled.button.attrs<ToggleProps>({
  type: 'button',
  role: 'switch'
})<ToggleProps>`
  --bg-color: ${(p) => p.theme.color.disabled};
  --border-color: transparent;
  --border-width: 2px;
  --animation-name: none;

  --width: 60px;
  --height: 30px;

  --knob-width: 24px;
  --knob-height: 24px;
  --knob-right: initial;
  --knob-left: initial;
  --knob-border-top: 1px;
  --knob-color: white;
  --knob-border-color: transparent;
  --knob-border-width: 1px;

  position: relative;
  display: inline-flex;
  width: var(--width);
  height: var(--height);
  border: var(--border-width) solid var(--border-color);
  border-radius: 50px;
  background: var(--bg-color);
  background-size: 400% 400%;

  ${p => p.isOn && css`
    --knob-left: initial;
    --knob-right: 2px;
    --knob-color: #4C54D2;
    --bg-color: #E1E2F6;

    @media (prefers-color-scheme: dark) {
      --knob-color: #4436E1;
      --bg-color: #7679B1;
    }
  `}

  ${p => !p.isOn && css`
    --knob-left: 2px;
    --knob-right: initial;
  `}

  // size variants
  ${({ size }) => {
    if (size === 'sm') {
      return css`
        --width: 48px;
        --height: 24px;
        --border-width: 2px;
        --knob-width: 16px;
        --knob-height: 16px;
        --knob-border-width: 2px;
      `
    }

    return css``
  }}

  // brand variants
  ${({ brand, isOn }) => {
    if (!isOn) return

    if (brand === 'shields' || brand === 'vpn') {
      return css`
        animation: ${moveBg} 5s ease infinite;
        --knob-color: white;
        --bg-color: linear-gradient(305.95deg, #381e85 0%, #6845d1 98.59%, #737ade 100%, #4d56d0 75%, #0e1bd1 100%);

        @media (prefers-color-scheme: dark) {
          --knob-color: white;
          --bg-color: linear-gradient(305.95deg, #381e85 0%, #6845d1 98.59%, #737ade 100%, #4d56d0 75%, #0e1bd1 100%);
        }
      `
    }

    return css``
  }}

  &:hover {
    --border-color: ${p => p.theme.color.interactive05};
  }

  &:focus-visible {
    --border-color: ${p => p.theme.color.focusBorder};
  }

  &:active {
    --border-color: transparent;
  }

  &:disabled,
  [disabled] {
    --knob-color: ${p => p.theme.color.subtle};
    --knob-border-color: #D9D9EA;
    --bg-color: ${(p) => p.theme.color.disabled};
  }

  // Knob
  &:after {
    content: '';
    box-sizing: border-box;
    position: absolute;
    top: var(--knob-border-width);
    left: var(--knob-left);
    right: var(--knob-right);
    width: var(--knob-width);
    height: var(--knob-height);
    border-radius: 50%;
    background-color: var(--knob-color);
    border: var(--knob-border-width) solid var(--knob-border-color);
  }
`

export const FLToggleBox = styled(ToggleBox)<ToggleProps>`
  --width: 96px;
  --height: 50px;
  --border-width: 3px;
  --knob-width: 40px;
  --knob-height: 40px;
  --knob-border-width: 2px;
`
