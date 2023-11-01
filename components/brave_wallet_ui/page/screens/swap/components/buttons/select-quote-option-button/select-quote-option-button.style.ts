// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledButton, StyledDiv } from '../../shared-swap.styles'

export const Button = styled(StyledButton)<{
  isSelected: boolean
}>`
  --best-background: ${(p) =>
    p.isSelected ? p.theme.color.interactive05 : p.theme.color.focusBorder};
  /* #f9faff does not exist in the design system */
  background-color: #f9faff;
  border-radius: 8px;
  justify-content: space-between;
  padding: 12px 24px;
  width: 100%;
  margin: 0px 0px 10px 10px;
  position: relative;
  box-sizing: border-box;
  box-shadow: ${(p) =>
    p.isSelected
      ? `0px 0px 0px 1px ${p.theme.color.interactive05} inset`
      : 'none'};
  &:hover {
    --best-background: ${(p) => p.theme.color.interactive05};
    box-shadow: 0px 0px 0px 1px ${(p) => p.theme.color.interactive05} inset;
  }
  @media (prefers-color-scheme: dark) {
    /* #222530 does not exist in the design system */
    background-color: #222530;
  }
`

export const BestOptionBadge = styled(StyledDiv)<{
  isSelected: boolean
}>`
  font-size: 12px;
  line-height: 20px;
  color: ${(p) => p.theme.palette.white};
  border-radius: 7px 7px 7px 0px;
  background-color: var(--best-background);
  padding: 0px 16px;
  position: absolute;
  z-index: 5;
  top: -9px;
  left: 0px;
`
