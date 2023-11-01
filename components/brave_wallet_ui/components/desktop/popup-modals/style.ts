// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import CloseIcon from '../../../assets/svg-icons/close.svg'
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
  z-index: 100;
  background: rgba(33, 37, 41, 0.32);
  backdrop-filter: blur(16px);
`

export const Modal = styled.div<{
  width?: string
  borderRadius?: number
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
  border-radius: ${(p) => (p.borderRadius ? p.borderRadius : 8)}px;
  box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.25);
  box-sizing: border-box;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    width: unset;
    min-width: unset;
    max-width: unset;
    max-height: unset;
    height: unset;
    position: absolute;
    border-radius: 16px 16px 0px 0px;
    top: 32px;
    bottom: 0px;
    left: 0px;
    right: 0px;
  }
`

export const Header = styled.div<{
  headerPaddingVertical?: number
  headerPaddingHorizontal?: number
}>`
  --vertical-padding: ${(p) => p.headerPaddingVertical ?? 20}px;
  --horizontal-padding: ${(p) => p.headerPaddingHorizontal ?? 20}px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  padding: var(--vertical-padding) var(--horizontal-padding);
  width: 100%;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 18px;
  font-weight: 600;
  letter-spacing: 0.02em;
  line-height: 26px;
  color: ${(p) => p.theme.color.text01};
  margin-right: 16px;
`

export const CloseButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${CloseIcon});
  mask-image: url(${CloseIcon});
  outline: none;
  border: none;
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
