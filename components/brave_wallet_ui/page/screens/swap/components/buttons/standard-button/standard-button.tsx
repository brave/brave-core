// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledButton } from '../../shared-swap.styles'

interface Props {
  isSelected?: boolean
  buttonSize?: 'normal' | 'small'
  buttonStyle?: 'round' | 'square'
  buttonType?: 'primary' | 'secondary'
  buttonWidth?: 'dynamic' | 'full' | number
  disabled?: boolean
  horizontalMargin?: number
  verticalMargin?: number
  marginRight?: number
}

export const StandardButton = styled(StyledButton)<Props>`
  --button-background: ${(p) =>
    p.buttonStyle === 'square'
      ? p.theme.color.divider01
      : p.buttonType === 'secondary'
      ? p.theme.color.background01
      : p.theme.color.interactive05};
  --button-background-hover: ${(p) =>
    p.buttonStyle === 'square'
      ? p.theme.color.interactive05
      : p.theme.color.interactive04};
  --vertical-margin: ${(p) => p.verticalMargin ?? 0}px;
  --horizontal-margin: ${(p) => p.horizontalMargin ?? 0}px;
  --border-secondary-selected: ${(p) => p.theme.color.interactive05};
  --button-color-disabled: #ffffff;
  @media (prefers-color-scheme: dark) {
    --button-background: ${(p) =>
      p.buttonStyle === 'square'
        ? p.theme.color.divider01
        : p.buttonType === 'secondary'
        ? p.theme.color.background01
        : p.theme.palette.blurple500};
    --button-background-hover: ${(p) =>
      p.buttonStyle === 'square'
        ? p.theme.palette.blurple500
        : p.theme.color.interactive04};
    --border-secondary-selected: ${(p) => p.theme.color.focusBorder};
    /* #677078 does not exist in design system */
    --button-color-disabled: #677078;
  }
  background-color: var(--button-background);
  border-radius: ${(p) => (p.buttonStyle === 'square' ? '0px' : '48px')};
  border: ${(p) =>
    p.buttonType === 'secondary'
      ? p.isSelected
        ? '1px solid var(--border-secondary-selected)'
        : `1px solid ${p.theme.color.interactive08}`
      : 'none'};
  color: ${(p) =>
    p.buttonType === 'secondary'
      ? p.isSelected
        ? 'var(--border-secondary-selected)'
        : p.theme.color.text03
      : p.buttonStyle === 'square'
      ? p.theme.color.text02
      : p.theme.palette.white};
  font-size: ${(p) =>
    p.buttonStyle === 'square' || p.buttonSize === 'small' ? '14px' : '16px'};
  margin: var(--vertical-margin) var(--horizontal-margin);
  margin-right: ${(p) => p.marginRight ?? 0}px;
  padding: ${(p) => (p.buttonSize === 'small' ? '6px 15px' : '18px')};
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
        ? '1px solid var(--border-secondary-selected)'
        : 'none'};
    color: ${(p) =>
      p.buttonType === 'secondary'
        ? p.isSelected
          ? 'var(--border-secondary-selected)'
          : p.theme.color.text03
        : p.theme.palette.white};
  }
  :disabled {
    background-color: ${(p) => p.theme.color.disabled};
    color: var(--button-color-disabled);
  }
`
