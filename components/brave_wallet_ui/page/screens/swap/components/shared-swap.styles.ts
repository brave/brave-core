// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import LeoIcon from '@brave/leo/react/icon'

export const Text = styled.span<{
  textSize?: '20px' | '18px' | '16px' | '14px' | '12px'
  responsiveTextSize?: '20px' | '18px' | '16px' | '14px' | '12px'
  isBold?: boolean
  textColor?: 'text01' | 'text02' | 'text03' | 'error' | 'success' | 'warning'
  maintainHeight?: boolean
  textAlign?: 'left' | 'right'
}>`
  --text01: ${(p) => p.theme.color.text01};
  --text02: ${(p) => p.theme.color.text02};
  --text03: ${(p) => p.theme.color.text03};
  --error: ${leo.color.red[40]};
  --success: ${leo.color.green[30]};
  --warning: ${leo.color.yellow[30]};
  font-family: 'Poppins';
  color: ${(p) =>
    p.textColor ? `var(--${p.textColor})` : p.theme.color.text01};
  font-size: ${(p) => (p.textSize ? p.textSize : '18px')};
  font-weight: ${(p) => (p.isBold ? 500 : 400)};
  height: ${(p) => (p.maintainHeight ? '20px' : 'unset')};
  line-height: ${(p) => (p.textSize === '12px' ? '18px' : '20px')};
  letter-spacing: 0.02em;
  text-align: ${(p) => (p.textAlign ? p.textAlign : 'center')};
  @media screen and (max-width: 570px) {
    font-size: ${(p) =>
      p.responsiveTextSize
        ? p.responsiveTextSize
        : p.textSize
        ? p.textSize
        : '18px'};
  }
`

export const StyledDiv = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  font-family: 'Poppins';
  color: ${(p) => p.theme.color.text01};
  font-weight: 400;
  font-size: 18px;
  line-height: 20px;
  letter-spacing: 0.02em;
`

export const Row = styled(StyledDiv)<{
  rowWidth?: 'dynamic' | 'full'
  rowHeight?: 'dynamic' | 'full'
  marginBottom?: number
  horizontalPadding?: number
  verticalPadding?: number
  verticalPaddingResponsive?: number
  horizontalAlign?: 'flex-start' | 'center' | 'flex-end'
  verticalAlign?: 'flex-start' | 'center' | 'flex-end'
}>`
  --vertical-padding: ${(p) => p.verticalPadding ?? 0}px;
  --horizontal-padding: ${(p) => p.horizontalPadding ?? 0}px;
  @media screen and (max-width: 570px) {
    --vertical-padding: ${(p) =>
      p.verticalPaddingResponsive ?? p.verticalPadding ?? 0}px;
  }
  box-sizing: border-box;
  flex-direction: row;
  align-items: ${(p) => p.verticalAlign ?? 'center'};
  justify-content: ${(p) => p.horizontalAlign ?? 'space-between'};
  margin-bottom: ${(p) => p.marginBottom ?? 0}px;
  padding: var(--vertical-padding) var(--horizontal-padding);
  width: ${(p) => (p.rowWidth === 'full' ? '100%' : 'unset')};
  height: ${(p) => (p.rowHeight === 'full' ? '100%' : 'unset')};
`

export const Column = styled(StyledDiv)<{
  columnWidth?: 'dynamic' | 'full'
  columnHeight?: 'dynamic' | 'full'
  horizontalAlign?: 'flex-start' | 'center' | 'flex-end'
  verticalAlign?: 'flex-start' | 'center' | 'flex-end'
  marginBottom?: number
  horizontalPadding?: number
  verticalPadding?: number
}>`
  --vertical-padding: ${(p) => p.verticalPadding ?? 0}px;
  --horizontal-padding: ${(p) => p.horizontalPadding ?? 0}px;
  align-items: ${(p) => p.horizontalAlign ?? 'center'};
  box-sizing: border-box;
  height: ${(p) => (p.columnHeight === 'full' ? '100%' : 'unset')};
  justify-content: ${(p) => p.verticalAlign ?? 'center'};
  margin-bottom: ${(p) => p.marginBottom ?? 0}px;
  padding: var(--vertical-padding) var(--horizontal-padding);
  width: ${(p) => (p.columnWidth === 'full' ? '100%' : 'unset')};
`

export const HorizontalSpacer = styled(StyledDiv)<{
  size: number
}>`
  height: 100%;
  width: ${(p) => p.size}px;
`

export const VerticalSpacer = styled(StyledDiv)<{
  size: number
}>`
  height: ${(p) => p.size}px;
  width: 100%;
`

export const HorizontalDivider = styled(StyledDiv)<{
  height?: number
  marginLeft?: number
  marginLeftResponsive?: number
  marginRight?: number
  dividerTheme?: 'lighter' | 'darker'
}>`
  background-color: ${(p) =>
    p.dividerTheme === 'darker'
      ? p.theme.color.interactive08
      : p.theme.color.divider01};
  height: ${(p) => (p.height ? `${p.height}px` : '100%')};
  margin-left: ${(p) => p.marginLeft ?? 0}px;
  margin-right: ${(p) => p.marginRight ?? 0}px;
  width: 2px;
  @media screen and (max-width: 570px) {
    margin-left: ${(p) => p.marginLeftResponsive ?? p.marginLeft ?? 0}px;
  }
`

export const VerticalDivider = styled(StyledDiv)<{
  width?: number
  marginTop?: number
  marginBottom?: number
}>`
  background-color: ${(p) => p.theme.color.divider01};
  height: 2px;
  margin-top: ${(p) => p.marginTop ?? 0}px;
  margin-bottom: ${(p) => p.marginBottom ?? 0}px;
  width: ${(p) => (p.width ? `${p.width}px` : '100%')};
`

export const Icon = styled(LeoIcon)<{
  size?: number
}>`
  --leo-icon-size: ${(p) => (p.size !== undefined ? p.size : 22)}px;
  color: ${leo.color.icon.default};
`

export const Loader = styled(StyledDiv)`
  animation: spin 0.75s linear infinite;
  border: 2px solid transparent;
  border-top: 2px solid ${(p) => p.theme.color.text03};
  border-radius: 50%;
  height: 10px;
  margin-right: 6px;
  width: 10px;
  @keyframes spin {
    0% {
      transform: rotate(0deg);
    }
    100% {
      transform: rotate(360deg);
    }
  }
`

export const StyledButton = styled.button`
  display: flex;
  font-family: 'Poppins';
  cursor: pointer;
  border: none;
  outline: none;
  background: none;
  font-size: 16px;
  font-weight: 500;
  line-height: 20px;
  letter-spacing: 0.02em;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  color: ${(p) => p.theme.color.text01};
  :disabled {
    cursor: not-allowed;
  }
`

export const IconButton = styled(StyledButton)`
  padding: 0px;
`

export const HiddenResponsiveRow = styled(Row)<{
  dontHide?: boolean
  maxWidth?: number
}>`
  display: flex;
  @media screen and (max-width: ${(p) => (p.maxWidth ? p.maxWidth : 800)}px) {
    display: ${(p) => (p.dontHide ? 'flex' : 'none')};
  }
`

export const ShownResponsiveRow = styled(Row)<{ maxWidth?: number }>`
  display: none;
  @media screen and (max-width: ${(p) => (p.maxWidth ? p.maxWidth : 800)}px) {
    display: flex;
  }
`

export const StyledInput = styled.input`
  font-family: 'Poppins';
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  outline: none;
  background-image: none;
  box-shadow: none;
  border: none;
  color: ${(p) => p.theme.color.text01};
  padding: 0px;
  -webkit-box-shadow: none;
  -moz-box-shadow: none;
  background-color: transparent;
  letter-spacing: 0.02em;
  ::placeholder {
    color: ${(p) => p.theme.color.text01};
  }
  :focus {
    outline: none;
  }
  ::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
  ::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
`

export const StyledLabel = styled.label`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
`

export const LPIcon = styled.div<{ icon: string; size?: string }>`
  background-image: url(${(p) => p.icon});
  background-size: cover;
  background-position: center;
  background-repeat: no-repeat;
  height: ${(p) => p.size ?? '20px'};
  width: ${(p) => p.size ?? '20px'};
  border-radius: ${leo.radius.full};
`

export const BankIcon = styled(LeoIcon).attrs({
  name: 'bank'
})<{ size?: string }>`
  --leo-icon-size: ${(p) => p.size ?? '20px'};
  color: ${leo.color.icon.default};
`
