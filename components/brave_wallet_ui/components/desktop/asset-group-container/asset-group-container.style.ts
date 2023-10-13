// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import {
  Column,
  Row,
  WalletButton,
  Text
} from '../../shared/style'
import {
  layoutPanelWidth
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled(Column) <{ hasBorder: boolean }>`
  border: ${p => p.hasBorder ? `1px solid ${leo.color.divider.subtle}` : 'none'};
  border-radius: 12px;
  margin-bottom: 16px;
  &:last-child {
    margin-bottom: 0px;
  }
`

export const CollapseButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 12px;
  margin: 0px;
  width: 100%;
  :disabled {
    cursor: default;
  }
`

export const CollapseIcon = styled(Icon) <
  {
    isCollapsed: boolean
  }>`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.interactive};
  transition-duration: 0.3s;
  transform: ${(p) =>
    p.isCollapsed
      ? 'unset'
      : 'rotate(180deg)'
  };
  margin-left: 16px;
`

export const AccountDescriptionWrapper = styled(Row)`
  align-items: center;
  justify-content: flex-start;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: flex-start;
    justify-content: center
  }
`

export const RewardsText = styled(Text)`
  margin: 0px 8px 0px 0px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    margin: 0px 0px 2px 0px;
  }
`

export const RewardsProviderContainer = styled(Row)`
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: flex-start;
    justify-content: center
  }
`
