// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { Icon, StyledDiv } from '../../shared.styles'

export const ButtonIcon = styled(Icon)`
  background-color: ${(p) => p.theme.color.text03};
  margin-right: 8px;
`

export const ArrowIcon = styled(Icon) <{ isOpen: boolean }>`
  background-color: ${(p) => p.theme.color.text02};
  transition-duration: 0.3s;
  transform: ${(p) => p.isOpen ? 'rotate(180deg)' : 'unset'};
`

export const DropDown = styled(StyledDiv)`
  background-color: ${(p) => p.theme.color.background02};
  padding: 8px;
  border-radius: 16px;
  position: absolute;
  width: 100%;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.18);
  }
  z-index: 10;
  top: 58px;
`
