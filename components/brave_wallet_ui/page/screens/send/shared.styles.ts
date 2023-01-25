// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Text = styled.span<{
  textSize?: '22px' | '20px' | '18px' | '16px' | '14px' | '12px'
  isBold?: boolean
  textColor?: 'text01' | 'text02' | 'text03' | 'success' | 'error'
  maintainHeight?: boolean
  textAlign?: 'left' | 'right'
}>`
  --text01: ${(p) => p.theme.color.text01};
  --text02: ${(p) => p.theme.color.text02};
  --text03: ${(p) => p.theme.color.text03};
  --success: ${(p) => p.theme.color.successIcon};
  --line-height: ${(p) => (p.textSize === '12px' ? '18px' : p.textSize === '22px' ? '24px' : '20px')}
  font-family: 'Poppins';
  color: ${(p) =>
    p.textColor ? `var(--${p.textColor})` : p.theme.color.text01};
  font-size: ${(p) => (p.textSize ? p.textSize : '18px')};
  font-weight: ${(p) => (p.isBold ? 500 : 400)};
  height: ${(p) => (p.maintainHeight ? 'var(--line-height)' : 'unset')};
  line-height: var(--line-height);
  letter-spacing: 0.02em;
  text-align: ${(p) => (p.textAlign ? p.textAlign : 'center')};
  word-wrap: wrap;
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

export const Row = styled(StyledDiv) <{
  rowWidth?: 'dynamic' | 'full'
  rowHeight?: 'dynamic' | 'full'
  marginBottom?: number
  horizontalPadding?: number
  verticalPadding?: number
  paddingTop?: number
  paddingBottom?: number
  paddingLeft?: number
  paddingRight?: number
  horizontalAlign?: 'flex-start' | 'center' | 'flex-end'
  verticalAlign?: 'flex-start' | 'center' | 'flex-end'
}>`
  --padding-top: ${(p) => p.paddingTop ?? p.verticalPadding ?? 0}px;
  --padding-bottom: ${(p) => p.paddingBottom ?? p.verticalPadding ?? 0}px;
  --padding-left: ${(p) => p.paddingLeft ?? p.horizontalPadding ?? 0}px;
  --padding-right: ${(p) => p.paddingRight ?? p.horizontalPadding ?? 0}px;
  box-sizing: border-box;
  flex-direction: row;
  align-items: ${(p) => p.verticalAlign ?? 'center'};
  justify-content: ${(p) => p.horizontalAlign ?? 'space-between'};
  margin-bottom: ${(p) => p.marginBottom ?? 0}px;
  padding: var(--padding-top) var(--padding-right) var(--padding-bottom) var(--padding-left);
  width: ${(p) => (p.rowWidth === 'full' ? '100%' : 'unset')};
  height: ${(p) => (p.rowHeight === 'full' ? '100%' : 'unset')};
`

export const Column = styled(StyledDiv) <{
  columnWidth?: 'dynamic' | 'full'
  columnHeight?: 'dynamic' | 'full'
  horizontalAlign?: 'flex-start' | 'center' | 'flex-end' | 'space-between'
  verticalAlign?: 'flex-start' | 'center' | 'flex-end' | 'space-between'
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

export const HorizontalSpacer = styled(StyledDiv) <{
  size: number
}>`
  height: 100%;
  width: ${(p) => p.size}px;
`

export const VerticalSpacer = styled(StyledDiv) <{
  size: number
}>`
  height: ${(p) => p.size}px;
  width: 100%;
`

export const HorizontalDivider = styled(StyledDiv) <{
  height?: number
  marginLeft?: number
  marginRight?: number
  dividerTheme?: 'lighter' | 'darker'
}>`
  --light-theme-color: #E9E9F4;
  @media (prefers-color-scheme: dark) {
  --light-theme-color: ${(p) => p.theme.color.interactive08};
  }
  background-color: ${(p) =>
    p.dividerTheme === 'lighter'
      ? 'var(--light-theme-color)'
      : p.theme.color.interactive08};
  height: ${(p) => (p.height ? `${p.height}px` : '100%')};
  margin-left: ${(p) => p.marginLeft ?? 0}px;
  margin-right: ${(p) => p.marginRight ?? 0}px;
  width: 2px;
`

export const VerticalDivider = styled(StyledDiv) <{
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

export const Icon = styled(StyledDiv) <{
  size: number
  icon: string
}>`
  -webkit-mask-image: url(${(p) => p.icon});
  height: ${(p) => p.size}px;
  mask-image: url(${(p) => p.icon});
  mask-size: contain;
  width: ${(p) => p.size}px;
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

export const IconButton = styled(StyledButton) <{
  size?: number
  icon: string
}>`
  background-color: ${(p) => p.theme.color.text02};
  height: ${(p) => (p.size ? p.size : 16)}px;
  mask-image: url(${(p) => p.icon});
  mask-size: contain;
  width: ${(p) => (p.size ? p.size : 16)}px;
  -webkit-mask-image: url(${(p) => p.icon});
`

export const HiddenResponsiveRow = styled(Row) <{ dontHide?: boolean }>`
  display: flex;
  @media screen and (max-width: 800px) {
    display: ${(p) => (p.dontHide ? 'flex' : 'none')};
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
