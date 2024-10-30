// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import LeoDialog from '@brave/leo/react/dialog'
import LeoDropdown from '@brave/leo/react/dropdown'
import Input from '@brave/leo/react/input'

// Shared Styles
import { WalletButton } from '../../../../../components/shared/style'
import {
  layoutPanelWidth //
} from '../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const Label = styled.label`
  color: ${color.text.tertiary};
  font: ${font.default.semibold};
  padding: 8px 0;
  margin: 0;
`

export const AssetSymbol = styled.h3`
  color: ${color.text.primary};
  text-align: center;
  font: ${font.heading.h3};
  padding: 0;
  margin: 0;
`

export const AssetIcon = styled.img<{ size?: string }>`
  width: ${(props) => props.size || '40px'};
  height: auto;
  border-radius: 50%;
`

export const CaretDown = styled(Icon).attrs({ name: 'carat-down' })`
  --leo-icon-color: ${color.icon.default};
  --leo-icon-size: 24px;
`

export const WrapperButton = styled(WalletButton)`
  background-color: transparent;
  border: none;
  cursor: pointer;
  padding: 0px;
  @media (max-width: ${layoutPanelWidth}px) {
    justify-content: space-between;
  }
`

export const ControlText = styled.h3`
  color: ${color.text.primary};
  text-align: left;
  font: ${font.heading.h3};
  padding: 0;
  margin: 0;
`

export const Dialog = styled(LeoDialog).attrs({
  size: window.innerWidth <= layoutPanelWidth ? 'mobile' : 'normal'
})`
  --leo-dialog-backdrop-background: rgba(17, 18, 23, 0.35);
  --leo-dialog-backdrop-filter: blur(8px);
  --leo-dialog-padding: 16px;
  .subtitle {
    border: 1px solid red;
    margin-bottom: 0;
  }
`

export const DialogTitle = styled.p`
  font: ${font.heading.h2};
  color: ${color.text.primary};
  text-align: left;
  margin: 0;
  @media (max-width: ${layoutPanelWidth}px) {
    font: ${font.heading.h3};
  }
`

export const ContainerButton = styled(WalletButton)`
  display: flex;
  padding: 16px;
  gap: 16px;
  width: 100%;
  justify-content: flex-start;
  align-items: center;
  background: transparent;
  border: none;
  outline: ${color.primary[70]};
  border-radius: 8px;
  cursor: pointer;
  &:hover {
    background: ${color.container.interactive};
  }
`

export const ListTitle = styled.span`
  color: ${color.text.tertiary};
  font: ${font.components.tableheader};
`

export const Dropdown = styled(LeoDropdown)`
  min-width: 100%;
`

export const SearchInput = styled(Input).attrs({
  mode: 'filled',
  size: window.innerWidth <= layoutPanelWidth ? 'small' : 'normal'
})`
  width: 100%;
`

export const IconsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
`

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  bottom: -3px;
  right: -3px;
  background-color: ${color.container.background};
  border-radius: 100%;
  padding: 2px;
  z-index: 3;
`
