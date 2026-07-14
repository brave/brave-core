// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton } from '../../../../components/shared/style'

export const OptionButton = styled(WalletButton)<{
  isSelected: boolean
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  background: none;
  cursor: pointer;
  outline: none;
  margin: 0px;
  background-color: ${({ isSelected }) =>
    isSelected
      ? leo.color.container.interactive
      : leo.color.container.background};
  border-radius: ${leo.radius.l};
  border: 2px solid
    ${({ isSelected }) =>
      isSelected ? leo.color.icon.interactive : leo.color.divider.subtle};
  padding: ${leo.spacing.xl};
  width: 100%;
  gap: 16px;
`

export const RadioIcon = styled(Icon)<{
  isSelected: boolean
}>`
  --leo-icon-size: 24px;
  --leo-icon-color: ${({ isSelected }) =>
    isSelected ? leo.color.icon.interactive : leo.color.icon.default};
`
