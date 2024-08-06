// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton, Column, Row } from '../../shared/style'
import { layoutPanelWidth } from '../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;
  position: fixed;
  z-index: 30;
  background: rgba(33, 37, 41, 0.32);
  backdrop-filter: blur(16px);
`

export const Modal = styled.div<{
  width?: string
  height?: string
}>`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-width: ${(p) => (p.width ? p.width : '580px')};
  max-width: ${(p) => (p.width ? p.width : '580px')};
  max-height: 90vh;
  height: ${(p) => p.height ?? 'unset'};
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.xl};
  box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.25);
  box-sizing: border-box;
  overflow: hidden;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    width: unset;
    min-width: unset;
    max-width: unset;
    position: absolute;
    border-radius: 16px 16px 0px 0px;
    bottom: 0px;
    left: 0px;
    right: 0px;
  }
`

export const ModalContent = styled(Column)`
  overflow-y: auto;
`

export const Header = styled.div<{
  headerPaddingVertical?: string
  headerPaddingHorizontal?: string
}>`
  --vertical-padding: ${(p) => p.headerPaddingVertical ?? '20px'};
  --horizontal-padding: ${(p) => p.headerPaddingHorizontal ?? '20px'};
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  padding: var(--vertical-padding) var(--horizontal-padding);
  width: 100%;
`

export const Title = styled.span`
  color: ${leo.color.text.primary};
  font: ${leo.font.heading.h2};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    font: ${leo.font.heading.h4};
  }
`

export const HeaderButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 24px;
  height: 24px;
  background-color: none;
  background: none;
  outline: none;
  border: none;
`

export const CloseIcon = styled(Icon).attrs({
  name: 'close'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const BackIcon = styled(Icon).attrs({
  name: 'arrow-left'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const Divider = styled.div`
  display: flex;
  width: 100%;
  border-bottom: 2px solid ${(p) => p.theme.color.divider01};
  padding-bottom: 6px;
`

export const PaddedRow = styled(Row)`
  padding: 0px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px;
  }
`

export const PaddedColumn = styled(Column)`
  padding: 0px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px;
  }
`
