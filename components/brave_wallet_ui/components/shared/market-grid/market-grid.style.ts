// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled, { css } from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

import {
  layoutPanelWidth,
  layoutSmallWidth
} from '../../desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { WalletButton, Text } from '../style'

export const breakpoints = {
  panel: `${layoutPanelWidth}px`,
  small: `${layoutSmallWidth}px`
}

export const StyledWrapper = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  width: 100%;
`

export const TableWrapper = styled.div`
  width: 100%;
`

export const GridContainer = styled.div`
  display: flex;
  flex-direction: column;
  overflow: hidden;
  height: 100%;
  width: 100%;
  overflow-y: hidden;
`

export const Header = styled.div<{ templateColumns: string }>`
  display: grid;
  grid-template-columns: ${({ templateColumns }) => templateColumns};
  grid-column-gap: 5px;
  padding-top: 16px;
  position: static;
  top: 0;
`

export const HeaderItem = styled.div<{
  hideOnPanel?: boolean
  hideOnSmall?: boolean
  sortable?: boolean
}>`
  display: flex;
  align-items: center;
  gap: ${({ sortable }) => (sortable ? '5px' : '0px')};
  cursor: ${({ sortable }) => (sortable ? 'pointer' : 'auto')};
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 18px;
  color: ${leo.color.text.tertiary};
  ${({ hideOnPanel }) =>
    hideOnPanel &&
    css`
      @media (max-width: ${breakpoints.panel}) {
        display: none;
      }
    `}
  ${({ hideOnSmall }) =>
    hideOnSmall &&
    css`
      @media (min-width: ${
          breakpoints.panel //
        }) and (max-width: ${
          breakpoints.small //
        }) {
        display: none;
      }
    `}
`
export const GridRowsWrapper = styled.div`
  overflow-y: auto;

  &::-webkit-scrollbar {
    display: none;
  }
`
export const GridRow = styled.div<{ templateColumns: string }>`
  display: grid;
  grid-template-columns: ${({ templateColumns }) => templateColumns};
  gap: 5px;
  margin-top: 16px;
  padding: 6px;
  cursor: pointer;
  border-radius: 10px;
  transition: background-color 300ms ease-out;
  &:hover {
    background-color: ${leo.color.page.background};
  }
`

export const Cell = styled.div<{
  hideOnPanel?: boolean
  hideOnSmall?: boolean
}>`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 500;
  line-height: normal;
  letter-spacing: 0.14px;
  color: ${leo.color.text.primary};
  overflow: hidden;
  ${({ hideOnPanel }) =>
    hideOnPanel &&
    css`
      @media (max-width: ${breakpoints.panel}) {
        display: none;
      }
    `}
  ${({ hideOnSmall }) =>
    hideOnSmall &&
    css`
      @media (min-width: ${
          breakpoints.panel //
        }) and (max-width: ${
          breakpoints.small //
        }) {
        display: none;
      }
    `}
`

export const SortIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.default};
`

export const AssetsColumnItemSpacer = styled.div`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  margin-right: 19px;
`
export const TextWrapper = styled.div<{
  alignment: 'right' | 'left' | 'center'
}>`
  display: flex;
  justify-content: ${(p) => {
    switch (p.alignment) {
      case 'left':
        return 'flex-start'
      case 'right':
        return 'flex-end'
      case 'center':
        return 'center'
      default:
        return 'center'
    }
  }};
  width: 100%;
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 500;
  line-height: normal;
  color: ${leo.color.text.primary};

  @media screen and (max-width: ${layoutPanelWidth}px) {
    font-size: 12px;
  }
`

export const ButtonsRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  position: relative;
  pointer-events: none;
`

export const CoinGeckoText = styled.span`
  font-family: Arial;
  font-size: 10px;
  font-weight: normal;
  color: ${leo.color.text.secondary};
  margin: 15px 0px;
`

export const ActionButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: transparent;
  border-radius: 48px;
  padding: 3px 10px;
  border: 1px solid ${leo.color.divider.interactive};
  margin-right: 6px;
  pointer-events: auto;
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 500;
  line-height: 16px;
  letter-spacing: 0.36px;
  color: ${leo.color.text.interactive};
`

export const EmptyStateText = styled(Text)`
  color: ${leo.color.text.secondary};
`
