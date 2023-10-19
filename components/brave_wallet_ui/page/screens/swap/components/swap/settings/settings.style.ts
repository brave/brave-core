// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import {
  StyledDiv,
  IconButton,
  StyledButton,
  Icon
} from '../../shared-swap.styles'

export const Modal = styled(StyledDiv)`
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 16px;
  border: 1px solid ${(p) => p.theme.color.disabled};
  box-shadow: 0px 4px 20px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
  flex-direction: column;
  justify-content: flex-start;
  overflow: hidden;
  position: absolute;
  padding: 20px 24px;
  width: 370px;
  min-height: 370px;
  z-index: 20;
  right: -16px;
  top: 28px;
  @media (prefers-color-scheme: dark) {
    border: 1px solid ${(p) => p.theme.color.divider01};
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  @media screen and (max-width: 570px) {
    position: fixed;
    right: 0;
    left: 0;
    top: 72px;
    bottom: 0;
    width: auto;
    border: none;
    border-radius: 16px 16px 0 0;
  }
`

export const ExchangesColumn = styled(StyledDiv)`
  display: grid;
  width: 100%;
  grid-template-columns: repeat(2, 1fr);
  grid-gap: 20px;
`

export const Button = styled(StyledButton)<{
  isSelected: boolean
}>`
  --border-selected: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    --border-selected: ${(p) => p.theme.color.focusBorder};
  }
  display: flex;
  justify-content: space-between;
  width: 100%;
  background-color: ${(p) => p.theme.color.background01};
  border-radius: 8px;
  border: 1px solid
    ${(p) =>
      p.isSelected ? 'var(--border-selected)' : p.theme.color.divider01};
  padding: 12px 16px;
  margin-bottom: 8px;
  &:hover {
    border: 1px solid var(--border-selected);
  }
`

export const IconWrapper = styled(StyledDiv)`
  /* rgb(225, 226, 246) does not exist in design system */
  --icon-background: rgba(225, 226, 246, 0.25);
  @media (prefers-color-scheme: dark) {
    /* rgb(118, 121, 177) does not exist in design system */
    --icon-background: rgba(118, 121, 177, 0.25);
  }
  width: 32px;
  height: 32px;
  border-radius: 100%;
  background-color: var(--icon-background);
  margin-right: 12px;
`

export const ButtonIcon = styled(Icon)`
  color: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.color.interactive06};
  }
`

export const MoreButton = styled(IconButton)<{
  isSelected: boolean
  expandOut?: boolean
}>`
  transform: ${(p) =>
    p.expandOut ? 'rotate(270deg)' : p.isSelected ? 'rotate(180deg)' : 'unset'};
  transition: transform 300ms ease;
`
