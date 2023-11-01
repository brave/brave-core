// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv, StyledButton, Icon } from '../../shared-swap.styles'

export const SelectButton = styled(StyledButton)`
  background-color: ${(p) => p.theme.color.background01};
  border: 1px solid ${(p) => p.theme.color.interactive08};
  border-radius: 4px;
  box-sizing: border-box;
  flex-direction: row;
  justify-content: space-between;
  padding: 7px 12px;
  min-width: 200px;
  :disabled {
    opacity: 0.3;
  }
`

export const SelectorWrapper = styled(StyledDiv)`
  position: relative;
`

export const SelectorBox = styled(StyledDiv)`
  background-color: ${(p) => p.theme.color.background01};
  min-width: 200px;
  position: absolute;
  z-index: 10;
  top: 36px;
  left: 0px;
  padding: 6px 4px 4px 4px;
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 8px;
  box-sizing: border-box;
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 24px rgba(0, 0, 0, 0.56);
  }
`

export const StyledCaratDownIcon = styled(Icon)`
  color: ${(p) => p.theme.color.text01};
`
