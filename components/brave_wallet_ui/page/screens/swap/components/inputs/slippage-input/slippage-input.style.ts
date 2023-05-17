// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv, StyledInput } from '../../shared-swap.styles'


export const InputWrapper = styled(StyledDiv)`
  --focus-border: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    --focus-border: ${(p) => p.theme.color.focusBorder};
  }
  display: flex;
  width: 94px;
  height: 32px;
  background-color: ${(p) => p.theme.color.background01};
  border: 1px solid ${(p) => p.theme.color.interactive08};
  border-radius: 4px;
  flex-direction: row;
  padding: 0px 12px;
  justify-content: space-between;
  box-sizing: border-box;
  &:focus-within {
    border: 1px solid var(--focus-border);
  }
`

export const CustomSlippageInput = styled(StyledInput)`
  width: 50px;
  border: none;
  font-weight: 400;
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
    font-size: 14px;
    font-weight: 200;
  }
  :disabled {
    ::placeholder {
      color: ${(p) => p.theme.color.disabled};
    }
  }
`
