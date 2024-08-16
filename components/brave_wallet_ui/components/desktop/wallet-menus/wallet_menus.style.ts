// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import NalaButtonMenu from '@brave/leo/react/buttonMenu'

// components
import { Icon } from '../../nala/icon'
import { Button } from '../../nala/button'

// shared styles
import { Row } from '../../shared/style'
import {
  layoutPanelWidth,
  layoutSmallWidth
} from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const ButtonMenu = styled(NalaButtonMenu)`
  --leo-menu-control-min-width: 220px;
`

export const MoreVerticalIcon = styled(Icon).attrs({
  name: 'more-vertical',
  size: '24px',
  color: leo.color.icon.default
})``

export const AddIcon = styled(Icon).attrs({ name: 'plus-add' })``

export const MenuButton = Button

// TODO: delete
export const SStyledWrapper = styled.div<{
  yPosition?: number
  right?: number
  padding?: string
}>`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: ${(p) => p.padding ?? '8px 8px 0px 8px'};
  background-color: ${leo.color.container.background};
  border-radius: 8px;
  border: 1px solid ${leo.color.divider.subtle};
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.25);
  position: absolute;
  top: ${(p) => (p.yPosition !== undefined ? p.yPosition : 35)}px;
  right: ${(p) => (p.right !== undefined ? p.right : 0)}px;
  z-index: 20;
`

export const PopupButton = styled('leo-menu-item')<{
  minWidth?: number // TODO: delete
}>``

export const MenuItemRow = styled.div`
  display: flex;
  justify-content: flex-start;
  align-items: center;
  gap: 16px;
`

export const MenuOptionRow = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 16px;
`

// TODO: delete
export const PopupButtonText = styled.span`
  flex: 1;
  font-family: Poppins;
  font-style: normal;
  font-size: 14px;
  font-weight: 400;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const MenuItemIcon = styled(Icon).attrs({ size: '18px' })``

export const INTERACTIVE_ICON_COLOR = leo.color.icon.interactive

export const ToggleRow = styled('leo-option').attrs({
  'data-is-interactive': true
})``

export const LineChartWrapper = styled(SStyledWrapper)`
  padding: 4px;
  gap: 4px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    left: 0px;
    right: unset;
  }
  @media screen and (max-width: ${layoutPanelWidth}px) {
    left: unset;
    right: 0px;
  }
`

export const LineChartButton = styled(PopupButton)`
  margin: 0px;
  padding: 6px 16px;
  &:hover {
    background-color: ${leo.color.page.background};
  }
`

export const SectionLabel = styled(Row)`
  background-color: ${leo.color.page.background};
  padding: 4px 8px;
  font: ${leo.font.components.label};
  text-transform: capitalize;
  color: ${leo.color.text.tertiary};
`
