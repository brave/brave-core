// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Dropdown from '@brave/leo/react/dropdown'
import LeoIcon from '@brave/leo/react/icon'

// Constants
import { layoutPanelWidth } from '../../../wallet-page-wrapper/wallet-page-wrapper.style'

// Shared Styles
import { Row, Column, WalletButton, Text } from '../../../../shared/style'

export const CheckboxRow = styled(Row)`
  flex-wrap: wrap;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: flex-start;
  }
`

export const CheckboxWrapper = styled(Row)`
  min-width: 50%;
`

export const SelectAllButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.03em;
  line-height: 16px;
  font-weight: 600;
  color: ${leo.color.text.interactive};
`

export const Title = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 28px;
`

export const Description = styled(Text)`
  color: ${leo.color.text.secondary};
  line-height: 18px;
`

export const CheckboxText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 24px;
`

export const IconWrapper = styled(Column)`
  width: 32px;
  height: 32px;
  background-color: ${leo.color.container.highlight};
  border-radius: 100%;
  margin-right: 16px;
`

export const Icon = styled(LeoIcon)`
  --leo-icon-size: 16px;
  color: ${leo.color.icon.default};
`

export const DropdownFilter = styled(Dropdown)`
  min-width: 40%;
  color: ${leo.color.text.primary};
`
