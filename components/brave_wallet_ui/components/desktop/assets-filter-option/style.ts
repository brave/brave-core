// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { WalletButton } from '../../shared/style'

export const Option = styled(WalletButton)<{
  selected?: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  text-align: left;
  cursor: pointer;
  border-radius: 8px;
  outline: none;
  border: none;
  background: none;
  padding: 12px 8px;
  margin: 0px 0px 8px 0px;
  background-color: transparent;
  &:hover {
    background-color: ${leo.color.divider.subtle};
  }
  font-weight: ${(p) => (p.selected ? 600 : 'normal')};
  cursor: pointer;
  width: 100%;
  color: ${leo.color.text.primary};
`
