// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'
import ProgressRing from '@brave/leo/react/progressRing'
import LeoDropdown from '@brave/leo/react/dropdown'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import {
  layoutPanelWidth //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { Column, Row } from '../../../components/shared/style'

export const ContentWrapper = styled(Column)`
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 8px;
  }
`

export const ControlPanel = styled(Row)`
  gap: 16px;
  justify-content: space-between;
  align-items: flex-start;
  overflow: hidden;
  flex-wrap: wrap;
  padding: 0px 12px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    border-radius: 16px;
    padding: 8px 20px 20px 20px;
    background-color: ${color.container.background};
    justify-content: flex-start;
    margin-bottom: 8px;
  }
`

export const ServiceProvidersWrapper = styled(Column)`
  flex: 1;
  gap: 16px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    gap: 8px;
    border-radius: 16px;
    background-color: ${color.container.background};
    padding: 8px;
  }
`

export const LoaderText = styled.p`
  color: ${color.text.primary};
  font: ${font.default.regular};
  text-align: center;
`

export const Loader = styled(ProgressRing).attrs({
  mode: 'indeterminate'
})`
  --leo-progressring-size: 32px;
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background-color: ${color.divider.subtle};
  margin: 40px 0px 24px 0px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    display: none;
  }
`

export const PaymentMethodIcon = styled.img`
  width: 20px;
  height: 20px;
  margin-right: 8px;
`

export const SearchAndFilterRow = styled(Row)`
  gap: 12px;
  flex-wrap: wrap;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    gap: 2px;
  }
`

export const SearchBarWrapper = styled(Row)`
  max-width: 200px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    max-width: unset;
  }
`

export const DropdownRow = styled(Row)`
  width: unset;
  gap: 12px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    width: 100%;
    gap: 4px;
  }
`

export const Dropdown = styled(LeoDropdown).attrs({
  size: window.innerWidth <= layoutPanelWidth ? 'small' : 'normal'
})`
  width: unset;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    width: 100%;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
`

export const InfoIconWrapper = styled(Column)`
  width: 56px;
  height: 56px;
  border-radius: 100%;
  background-color: ${color.page.background};
`

export const InfoIcon = styled(Icon).attrs({ name: 'info-outline' })`
  --leo-icon-color: ${color.icon.default};
  --leo-icon-size: 24px;
`
