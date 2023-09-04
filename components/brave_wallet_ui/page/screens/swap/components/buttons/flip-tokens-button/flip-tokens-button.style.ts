// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv, StyledButton, Icon } from '../../shared-swap.styles'

export const Wrapper = styled(StyledDiv)`
  height: 8px;
  width: 100%;
`

export const Button = styled(StyledButton)`
  --icon-color: ${(p) => p.theme.color.text02};
  --icon-color-hover: ${(p) => p.theme.color.interactive05};
  /* #f0f1fc does not exist in the design system */
  --background-color-hover: #f0f1fc;
  background-color: ${(p) => p.theme.color.background01};
  border-radius: 100%;
  box-shadow: 0px 0px 10px rgba(99, 105, 110, 0.2);
  height: 40px;
  position: absolute;
  width: 40px;
  z-index: 10;
  padding: 0px;
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
    --icon-color-hover: ${(p) => p.theme.color.focusBorder};
    /* #484b67 does not exist in the design system */
    --background-color-hover: #484b67;
  }
  &:hover {
    --icon-color: var(--icon-color-hover);
    background-color: var(--background-color-hover);
  }
`

export const ButtonIcon = styled(Icon)`
  color: var(--icon-color);
`
