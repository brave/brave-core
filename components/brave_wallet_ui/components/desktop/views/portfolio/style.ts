// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Icons
import {
  NoAccountsIconDark,
  NoAccountsIconLight,
  NoTransactionsIconDark,
  NoTransactionsIconLight
} from '../../../../assets/svg-icons/empty-state-icons'

// Shared Styles
import {
  AssetIconProps,
  AssetIconFactory,
  WalletButton,
  Row,
  Column
} from '../../../shared/style'
import {
  layoutSmallWidth,
  layoutPanelWidth
} from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100%;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
`

export const BalanceText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 36px;
  font-weight: 500;
  line-height: 54px;
  color: ${leo.color.text.primary};
`

export const FiatChange = styled.span<{ isDown?: boolean }>`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  font-weight: 400;
  line-height: 18px;
  margin-right: 8px;
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.successIcon};
`

export const ButtonRow = styled.div<{
  noMargin?: boolean
  horizontalPadding?: number
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin: ${(p) => (p.noMargin ? '0px' : '20px 0px')};
  padding: 0px
    ${(p) => (p.horizontalPadding !== undefined ? p.horizontalPadding : 0)}px;
  gap: 10px;
`

export const BalanceRow = styled.div<{ gap?: string }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  height: 36px;
  vertical-align: middle;
  gap: ${(p) => p.gap || 0};
`

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const SubDivider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
  margin-bottom: 12px;
`

export const DividerText = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  font-weight: 600;
  margin-bottom: 10px;
  color: ${(p) => p.theme.color.text03};
`

export const PercentBubble = styled.div<{ isDown?: boolean }>`
  display: flex;
  padding: 4px 8px;
  border-radius: 4px;
  background-color: ${(p) =>
    p.isDown ? leo.color.red[20] : leo.color.green[20]};
  font-family: Poppins;
  font-size: 11px;
  line-height: 16px;
  letter-spacing: 0.02em;
  font-weight: 500;
  color: ${(p) => (p.isDown ? leo.color.red[50] : leo.color.green[50])};
`

export const Spacer = styled.div`
  display: flex;
  height: 2px;
  width: 100%;
  margin-top: 10px;
`

export const CoinGeckoText = styled.span`
  font-family: Arial;
  font-size: 10px;
  font-weight: normal;
  color: ${(p) => p.theme.color.text03};
  margin: 15px 0px;
`

export const FilterTokenRow = styled.div<{
  horizontalPadding?: number
  isV2?: boolean
}>`
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  width: 100%;
  gap: 14px;
  padding: 0px
    ${(p) => (p.horizontalPadding !== undefined ? p.horizontalPadding : 0)}px;
  margin-bottom: ${(p) => (p.isV2 ? '16px' : 0)};
`

export const SelectTimelineWrapper = styled(Row)`
  @media screen and (max-width: ${layoutSmallWidth}px) {
    justify-content: flex-start;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    justify-content: center;
  }
`

export const ControlsRow = styled(Row)<{ controlsHidden?: boolean }>`
  box-shadow: 0px -1px 1px ${leo.color.elevation.primary};
  border-radius: 16px 16px 0px 0px;
  padding: ${(p) => (p.controlsHidden ? '12px' : '24px 32px')};
  background-color: ${leo.color.container.background};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: ${(p) => (p.controlsHidden ? '8px' : '16px')};
  }
`

export const BalanceAndButtonsWrapper = styled(Column)`
  @media screen and (max-width: ${layoutSmallWidth}px) {
    flex-direction: row;
    justify-content: space-between;
    align-items: flex-start;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    justify-content: flex-start;
    align-items: center;
    padding: 24px 0px;
  }
`

export const BalanceAndChangeWrapper = styled(Column)`
  position: relative;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    align-items: flex-start;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    margin-bottom: 24px;
  }
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
  border-radius: 8px;
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

export const EmptyStateIcon = styled.div`
  width: 100px;
  height: 100px;
  background-repeat: no-repeat;
  background-size: 100%;
  background-position: center;
  margin-bottom: 16px;
`

export const EmptyTransactionsIcon = styled(EmptyStateIcon)`
  background-image: url(${NoTransactionsIconLight});
  @media (prefers-color-scheme: dark) {
    background-image: url(${NoTransactionsIconDark});
  }
`

export const EmptyAccountsIcon = styled(EmptyStateIcon)`
  background-image: url(${NoAccountsIconLight});
  @media (prefers-color-scheme: dark) {
    background-image: url(${NoAccountsIconDark});
  }
`

export const ToggleVisibilityButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  pointer-events: auto;
  border: none;
`

export const EyeIcon = styled(Icon)`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`

export const ContentWrapper = styled(Column)<{
  isPanel: boolean
}>`
  background-color: ${(p) =>
    p.isPanel ? leo.color.container.background : 'transparent'};
`

export const BalanceAndLineChartWrapper = styled(Column)`
  position: relative;
`

export const ActivityWrapper = styled(Column)`
  background-color: ${leo.color.container.background};
`
