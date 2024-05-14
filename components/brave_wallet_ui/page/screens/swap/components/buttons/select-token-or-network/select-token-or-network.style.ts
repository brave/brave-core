// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { StyledButton, Icon, Row, Text } from '../../shared-swap.styles'

export const Button = styled(StyledButton)<SelectTokenButtonStyleProps>`
  /* Variables */
  --big-padding: 10px ${(p) => (p.moreRightPadding ? 12 : 10)}px 10px 12px;
  --medium-padding: 8px 16px;
  --small-padding: 4px 12px 4px 4px;
  --background-color-hover-primary: rgba(255, 255, 255, 0.6);
  --background-color-hover-secondary: ${leo.color.purple[10]};

  @media screen and (max-width: 570px) {
    --big-padding: 10px ${(p) => (p.moreRightPadding ? 10 : 8)}px 10px 8px;
    --medium-padding: 4px 16px 4px 8px;
    --small-padding: 4px 16px 4px 8px;
  }

  @media (prefers-color-scheme: dark) {
    --background-color-hover-primary: ${(p) => p.theme.color.background01};
    --background-color-hover-secondary: ${(p) => p.theme.color.background02};
  }

  /* Styles */
  background-color: ${(p) =>
    p.hasBackground ? p.theme.color.background01 : 'transparent'};
  border-radius: 100px;
  box-shadow: ${(p) =>
    p.hasShadow ? '0px 0px 10px rgba(0, 0, 0, 0.05)' : 'none'};
  justify-content: ${(p) =>
    p.buttonSize === 'small' ? 'space-between' : 'center'};
  padding: ${(p) =>
    p.buttonSize === 'small'
      ? 'var(--small-padding)'
      : p.buttonSize === 'medium'
      ? 'var(--medium-padding)'
      : 'var(--big-padding)'};
  white-space: nowrap;
  width: ${(p) => (p.buttonSize === 'small' ? '140px' : 'unset')};
  :disabled {
    opacity: 0.3;
  }
  &:hover:not([disabled]) {
    background-color: ${(p) =>
      p.buttonType === 'secondary' || p.buttonSize === 'small'
        ? 'var(--background-color-hover-secondary)'
        : 'var(--background-color-hover-primary)'};
  }
`

export const ButtonIcon = styled(Icon)`
  color: ${(p) => p.theme.color.text01};
`

export const FuelTank = styled(Icon)`
  color: ${(p) => p.theme.color.text02};
  margin-right: 6px;
`

export const GasBubble = styled(Row)`
  padding: 2px 8px;
  border-radius: 8px;
  background-color: ${leo.color.purple[10]};
  @media (prefers-color-scheme: dark) {
    /* #282B37 does not exist in the design system */
    background-color: #282b37;
  }
`

export const NotSupportedText = styled(Text)`
  color: ${(p) => p.theme.palette.white};
`

export interface SelectTokenButtonStyleProps {
  buttonType?: 'primary' | 'secondary'
  buttonSize?: 'big' | 'medium' | 'small'
  moreRightPadding?: boolean
  hasBackground?: boolean
  hasShadow?: boolean
}
