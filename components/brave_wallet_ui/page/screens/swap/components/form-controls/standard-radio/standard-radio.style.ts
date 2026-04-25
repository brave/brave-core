// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledInput, StyledDiv, StyledLabel } from '../../shared-swap.styles'

export const Radio = styled(StyledInput)`
  --checked-border: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    --checked-border: ${(p) => p.theme.color.interactive06};
  }
  -webkit-appearance: none;
  appearance: none;
  margin: 0;
  width: 20px;
  height: 20px;
  border: 2px solid ${(p) => p.theme.color.interactive08};
  cursor: pointer;
  border-radius: 100%;
  ::after {
    content: '';
    display: block;
    border-radius: 100%;
    width: 10px;
    height: 10px;
    margin: 3px;
  }
  :checked {
    border: 2px solid var(--checked-border);
    ::after {
      background-color: var(--checked-border);
    }
  }
`

export const Wrapper = styled(StyledDiv)`
  flex-direction: row;
  gap: 12px;
`

export const Label = styled(StyledLabel)<{ isChecked: boolean }>`
  cursor: pointer;
  color: ${(p) => (p.isChecked ? p.theme.color.text01 : p.theme.color.text03)};
`
