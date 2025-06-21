// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { Column, Row, WalletButton, Text } from '../../shared/style'
import { layoutPanelWidth } from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled(Column)<{ isCollapsed: boolean }>`
  border: 1px solid
    ${(p) =>
      p.isCollapsed ? leo.color.container.highlight : leo.color.divider.subtle};
  border-radius: 12px;
  margin-bottom: 16px;
  &:last-child {
    margin-bottom: 0px;
  }
`

export const CollapsedWrapper = styled.div<{
  isCollapsed?: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  outline: none;
  background: none;
  border: none;
  margin: 0px;
  width: 100%;
  border-radius: ${(p) => (p.isCollapsed ? '11px' : '11px 11px 0px 0px')};
  background-color: ${leo.color.container.highlight};
`

export const CollapseIcon = styled(Icon)<{
  isCollapsed: boolean
}>`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
  transition-duration: 0.3s;
  transform: ${(p) => (p.isCollapsed ? 'unset' : 'rotate(180deg)')};
  margin-left: 16px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
`

export const AccountDescriptionWrapper = styled(Row)`
  height: 100%;
  align-items: center;
  justify-content: flex-start;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: flex-start;
    justify-content: center;
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
    justify-content: center;
  }
`

export const InfoBar = styled(Row)`
  background-color: ${leo.color.systemfeedback.warningBackground};
  border-radius: 12px;
  padding: 8px 16px;
`

export const InfoText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.systemfeedback.warningText};
`

export const WarningIcon = styled(Icon).attrs({
  name: 'warning-triangle-filled',
})`
  --leo-icon-size: 20px;
  color: ${leo.color.systemfeedback.warningIcon};
`

export const ClickArea = styled(WalletButton)`
  flex: 1;
  cursor: pointer;
  display: flex;
  flex-direction: row;
  background: none;
  border: none;
  margin: 0;
  padding: 0;
  :disabled {
    cursor: default;
  }
`

export const AccountIconClickArea = styled(ClickArea)`
  height: 100%;
  padding: 12px 0px 12px 12px;
`

export const AccountNameClickArea = styled(ClickArea)<{
  hasAddress?: boolean
}>`
  height: 100%;
  align-items: center;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    align-items: ${(p) => (p.hasAddress ? 'flex-end' : 'center')};
    height: unset;
  }
`

export const BalanceClickArea = styled(ClickArea)`
  height: 100%;
  padding-right: 12px;
  justify-content: flex-end;
`

export const AddressArea = styled(Row)`
  height: 100%;
  justify-content: flex-start;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex: 1;
    align-items: flex-start;
    height: unset;
    width: 100%;
  }
`

export const EmptyClickArea = styled(ClickArea)`
  display: none;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    display: flex;
    height: 100%;
  }
`

export const NetworkAndExternalProviderClickArea = styled(ClickArea)`
  padding: 12px 0px 12px 12px;
`
