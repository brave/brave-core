// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { WalletButton } from '../../../../components/shared/style'

export const Button = styled(WalletButton)<{
  isSelected: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  cursor: pointer;
  font-family: 'Poppins';
  outline: none;
  border: none;
  background-color: none;
  background: none;
  font-weight: 600;
  font-size: 14px;
  line-height: 22px;
  padding: 10px 0px 0px 0px;
  color: ${(p) =>
    p.isSelected ? leo.color.text.interactive : leo.color.text.secondary};
`

export const ButtonText = styled.span`
  margin: 0px 12px 5px 12px;
`

export const LineIndicator = styled.div<{
  isSelected: boolean
}>`
  width: 100%;
  height: 4px;
  border-radius: 4px;
  background-color: ${(p) =>
    p.isSelected ? leo.color.text.interactive : 'none'};
`
