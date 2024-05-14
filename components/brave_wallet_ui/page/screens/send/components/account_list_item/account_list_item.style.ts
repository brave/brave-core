// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { StyledButton, Text } from '../../shared.styles'

export const Button = styled(StyledButton)`
  --button-shadow-hover: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    --button-shadow-hover: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  width: 100%;
  border-radius: 10px;
  padding: 10px 8px;
  justify-content: flex-start;
  align-items: center;
  flex-direction: row;
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
  &:hover:not([disabled]) {
    box-shadow: var(--button-shadow-hover);
  }
`

export const NameAndBalanceText = styled(Text)`
  word-break: break-all;
  line-height: 22px;
  color: ${leo.color.text.primary};
  margin-bottom: 2px;
`

export const AddressText = styled(Text)`
  word-break: break-all;
  line-height: 20px;
  color: ${leo.color.text.secondary};
`

export const DisabledLabel = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  background-color: ${leo.color.green[20]};
  color: ${leo.color.green[50]};
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
  font-family: 'Poppins';
  line-height: 12px;
  padding: 4px 6px;
  border-radius: 4px;
  margin-left: 8px;
`
