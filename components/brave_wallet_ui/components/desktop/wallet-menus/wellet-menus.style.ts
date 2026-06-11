// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import LeoButtonMenu from '@brave/leo/react/buttonMenu'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Types
import { AccountModalTypes } from '../../../constants/types'

// Shared Styles
import { Text, WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div<{
  yPosition?: number
  right?: number
  left?: number
  padding?: string
  /** When true, anchor the menu above the positioning context (e.g. asset row). */
  $openUpward?: boolean
  /** Gap between menu and anchor when opening upward (px). */
  $verticalGap?: number
  /** When set, caps height and scrolls overflowing content. */
  $maxHeightPx?: number
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
  ${(p) => {
    const maxH = p.$maxHeightPx
    if (maxH === undefined) {
      return ''
    }
    return css`
      align-items: stretch;
      justify-content: flex-start;
      max-height: ${maxH}px;
      overflow-x: hidden;
      overflow-y: auto;
      -webkit-overflow-scrolling: touch;
    `
  }}
  ${(p) =>
    p.$openUpward
      ? css`
          top: auto;
          bottom: calc(100% + ${p.$verticalGap ?? 8}px);
        `
      : css`
          top: ${p.yPosition ?? 35}px;
          bottom: auto;
        `}
  right: ${(p) => {
    if (p.left !== undefined) {
      return 'unset'
    }
    if (p.right !== undefined) {
      return `${p.right}px`
    }
    return '0px'
  }};
  left: ${(p) => {
    if (p.right !== undefined) {
      return 'unset'
    }
    if (p.left !== undefined) {
      return `${p.left}px`
    }
    return 'unset'
  }};
  z-index: 20;
`

export const PopupButton = styled(WalletButton)<{
  minWidth?: number
}>`
  display: flex;
  flex-shrink: 0;
  align-items: center;
  justify-content: flex-start;
  text-align: left;
  cursor: pointer;
  min-width: ${(p) => p.minWidth ?? 220}px;
  border-radius: 8px;
  outline: none;
  border: none;
  background: none;
  padding: 12px 8px;
  margin: 0px 0px 8px 0px;
  background-color: transparent;
  width: 100%;
  &:hover {
    background-color: ${leo.color.divider.subtle};
  }
`

export const PopupButtonText = styled(Text).attrs({
  textAlign: 'left',
  variant: 'default.regular',
  textColor: 'primary',
})`
  flex: 1;
`

export const ButtonIcon = styled(Icon)<{ id?: AccountModalTypes }>`
  --leo-icon-size: 18px;
  color: ${(p) =>
    p.id === 'shield'
      ? leo.color.systemfeedback.successIcon
      : leo.color.icon.default};
  margin-right: 16px;
`

export const ButtonMenu = styled(LeoButtonMenu)<{
  minWidth?: number
}>`
  color: ${leo.color.icon.default};
  leo-menu-item {
    --leo-icon-size: 18px;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 16px;
    color: ${leo.color.text.primary};
    font: ${leo.font.default.regular};
    min-width: ${(p) => p.minWidth ?? 220}px;
    padding: 12px;
  }
  leo-menu-item #shield {
    --leo-icon-color: ${leo.color.systemfeedback.successIcon};
  }
  leo-menu-item#toggle {
    justify-content: space-between;
  }
`
