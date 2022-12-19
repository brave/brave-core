// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Assets
import { LoaderIcon } from 'brave-ui/components/icons'

// Shared Styles
import { StyledButton, Icon } from '../../shared.styles'

export interface StandardButtonStyleProps {
  buttonType?: 'primary' | 'secondary' | 'danger'
  buttonWidth?: 'dynamic' | 'full'
  disabled?: boolean
  marginRight?: number
  hasError?: boolean
}

export const Button = styled(StyledButton) <StandardButtonStyleProps>`
  --disabled-text-color: ${(p) => p.theme.palette.white};
  --button-background-primary: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    /* #677078 does not exist in design system */
    --disabled-text-color: #677078;
    --button-background-primary: ${(p) => p.theme.palette.blurple500};
  }
  --button-background: ${(p) =>
    p.buttonType === 'secondary'
      ? p.theme.color.background01
      : 'var(--button-background-primary)'};
  --button-background-hover: ${(p) => p.theme.color.interactive04};
  background-color: var(--button-background);
  border-radius: 48px;
  border: ${(p) =>
    p.buttonType === 'secondary'
      ? `1px solid ${p.theme.color.interactive08}`
      : 'none'};
  color: ${(p) =>
    p.buttonType === 'secondary'
      ? p.theme.color.text03
      : p.theme.palette.white};
  font-size: 16px;
  margin-right: ${(p) => p.marginRight ?? 0}px;
  padding: 18px;
  width: ${(p) =>
    p.buttonWidth === 'dynamic'
      ? 'unset'
      : p.buttonWidth === 'full'
        ? '100%'
        : `${p.buttonWidth}px`};
  &:hover:not([disabled]) {
    background-color: ${(p) =>
    p.buttonType === 'secondary'
      ? p.theme.color.background01
      : 'var(--button-background-hover)'};
    border: ${(p) =>
    p.buttonType === 'secondary'
      ? `1px solid ${p.theme.color.interactive05}`
      : 'none'};
    color: ${(p) =>
    p.buttonType === 'secondary'
      ? p.theme.color.text03
      : p.theme.palette.white};
  }
  :disabled {
    background-color: ${(p) =>
    p.hasError
      ? p.theme.color.errorBackground
      : p.theme.color.disabled};
    color: ${(p) =>
    p.hasError
      ? p.theme.color.errorBorder
      : 'var(--disabled-text-color)'};
  }
`

export const ErrorIcon = styled(Icon)`
  background-color: ${(p) => p.theme.color.errorBorder};
  margin-right: 12px;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.white};
  height: 25px;
  width: 25px;
  opacity: .4;
  margin-right: 10px;
`
