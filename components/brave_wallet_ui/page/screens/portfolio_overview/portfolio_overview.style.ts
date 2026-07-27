// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  Text,
  Row,
  Column,
  WalletButton,
} from '../../../components/shared/style'
import {
  layoutPanelWidth, //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const FiatChange = styled(Text)`
  margin-right: 8px;
`

export const PercentBubble = styled.div<{ isDown?: boolean }>`
  font: ${leo.font.xSmall.regular};
  display: flex;
  padding: 4px 8px;
  border-radius: 4px;
  background-color: ${(p) =>
    p.isDown ? leo.color.red[20] : leo.color.green[20]};
  letter-spacing: 0.02em;
  color: ${(p) => (p.isDown ? leo.color.red[50] : leo.color.green[50])};
`

export const ControlsRow = styled(Row)`
  box-shadow: 0px -1px 1px ${leo.color.elevation.primary};
  border-radius: 16px 16px 0px 0px;
  padding: 24px 32px;
  background-color: ${leo.color.container.background};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px;
  }
`

export const BalanceAndButtonsWrapper = styled(Column)<{
  hasZeroBalance?: boolean
}>`
  ${({ hasZeroBalance }) =>
    hasZeroBalance
      ? `
    flex-direction: column;
    justify-content: flex-start;
    align-items: center;
    padding: 40px 32px 24px 32px;
  `
      : `
    flex-direction: row;
    justify-content: space-between;
    align-items: flex-start;
    padding: 40px 32px;
  `}
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    justify-content: flex-start;
    align-items: center;
    padding: 24px 0px;
  }
`

export const BalanceAndChangeWrapper = styled(Column)<{
  hasZeroBalance?: boolean
}>`
  position: relative;
  align-items: flex-start;
  margin-bottom: 0px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    margin-bottom: ${(p) => (p.hasZeroBalance ? '0px' : '24px')};
  }
`

export const BalanceAndLineChartWrapper = styled(Column)`
  position: relative;
`

export const ActivityWrapper = styled(Column)<{
  isMobileOrPanel: boolean
}>`
  padding: 0px 32px 32px 32px;
  background-color: ${(p) =>
    p.isMobileOrPanel ? leo.color.container.background : 'transparent'};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px 16px 16px;
  }
`

export const ContentWrapper = styled(Column)<{
  isMobileOrPanel: boolean
}>`
  background-color: ${(p) =>
    p.isMobileOrPanel ? leo.color.container.background : 'transparent'};
`

export const PortfolioActionButton = styled(WalletButton)`
  --button-border-color: ${leo.color.divider.interactive};
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.full};
  border: 1px solid var(--button-border-color);
  height: 36px;
  width: 36px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    height: 28px;
    width: 28px;
  }
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.interactive};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    --leo-icon-size: 16px;
  }
`

export const SearchBarWrapper = styled(Row)<{
  showSearchBar: boolean
}>`
  width: 230px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    display: ${(p) => (p.showSearchBar ? 'flex' : 'none')};
    width: 100%;
  }
`

export const ControlBarWrapper = styled(Row)<{
  showSearchBar: boolean
  isNFTView?: boolean
}>`
  padding: 0px 32px;
  margin-bottom: 16px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: ${(p) => (p.showSearchBar ? (p.isNFTView ? '2px' : '0px') : '4px')}
      16px 0px 16px;
    margin-bottom: ${(p) => (p.showSearchBar ? 12 : 16)}px;
  }
`

export const SearchButtonWrapper = styled(Row)`
  display: none;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    display: flex;
  }
`
