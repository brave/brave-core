// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledInput, StyledDiv, StyledLabel } from '../../shared-swap.styles'

export const Label = styled(StyledLabel)`
  display: flex;
  align-items: center;
  cursor: pointer;
`

export const Switch = styled(StyledDiv)`
  position: relative;
  box-sizing: border-box;
  width: 48px;
  height: 24px;
  background: ${(p) => p.theme.color.disabled};
  border-radius: 32px;
  padding: 2px;
  transition: 300ms all;
  @media (prefers-color-scheme: dark) {
    background: ${(p) => p.theme.color.interactive08};
  }

  &:before {
    transition: 300ms all;
    content: '';
    position: absolute;
    width: 20px;
    height: 20px;
    border-radius: 35px;
    top: 50%;
    left: 4px;
    background: ${(p) => p.theme.color.background01};
    transform: translate(0, -50%);
  }
`

export const Input = styled(StyledInput)`
  /* #e1e2f6 Does not exist in the design system */
  --checked-background: #e1e2f6;
  --unchecked-background: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    /* #7679B1 Does not exist in the design system */
    --checked-background: #7679b1;
    /* #4436E1 Does not exist in the design system */
    --unchecked-background: #4436e1;
  }

  display: none;

  &:checked + ${Switch} {
    background: var(--checked-background);

    &:before {
      transform: translate(20px, -50%);
      background: var(--unchecked-background);
    }
  }
`
