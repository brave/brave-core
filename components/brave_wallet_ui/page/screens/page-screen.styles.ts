// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import LeoAlertCenter from '@brave/leo/react/alertCenter'
import { Column, Row, WalletButton } from '../../components/shared/style'
import { layoutPanelWidth } from '../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

import {
  NoTransactionsIconDark,
  NoTransactionsIconLight,
} from '../../assets/svg-icons/empty-state-icons'

export const AlertCenter = styled(LeoAlertCenter)`
  @media screen and (max-width: ${layoutPanelWidth}px) {
    --leo-alert-center-width: 95%;
  }
`

export const SimplePageWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
`

export const FullScreenWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  background-color: ${leo.color.container.background};
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

export const ContentWrapper = styled(Column)<{
  isMobileOrPanel: boolean
}>`
  background-color: ${(p) =>
    p.isMobileOrPanel ? leo.color.container.background : 'transparent'};
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
