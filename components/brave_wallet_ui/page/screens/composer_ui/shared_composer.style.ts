// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import Alert from '@brave/leo/react/alert'
import LeoButton from '@brave/leo/react/button'

// Shared Styles
import {
  Column,
  WalletButton,
  Row,
  AssetIconFactory,
  AssetIconProps,
  Text
} from '../../../components/shared/style'
import {
  layoutPanelWidth //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const ToSectionWrapper = styled(Column)<{
  tokenColor?: string
}>`
  --default-background: ${leo.color.container.interactive};
  @media (prefers-color-scheme: dark) {
    --default-background: ${leo.color.container.highlight};
  }
  padding: 0px 24px 24px 24px;
  border-radius: 0px 0px 24px 24px;
  background-color: ${(p) => p.tokenColor ?? 'var(--default-background)'};
  @media screen and (max-width: ${layoutPanelWidth}px) {
    border-radius: 0px;
    padding: 0px 16px 16px 16px;
    height: 100%;
  }
`

export const Input = styled.input`
  font-family: 'Poppins';
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  outline: none;
  background-image: none;
  box-shadow: none;
  border: none;
  color: ${leo.color.text.primary};
  padding: 0px;
  -webkit-box-shadow: none;
  -moz-box-shadow: none;
  background-color: transparent;
  ::placeholder {
    color: ${leo.color.text.tertiary};
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

export const AmountInput = styled(Input)<{
  hasError: boolean
}>`
  color: ${(p) =>
    p.hasError ? leo.color.systemfeedback.errorText : leo.color.text.primary};
  font-weight: 500;
  font-size: 32px;
  line-height: 42px;
  text-align: right;
  width: 100%;
`

export const PresetButton = styled(WalletButton)`
  outline: none;
  border: none;
  background-color: ${leo.color.neutral[10]};
  border-radius: 4px;
  font-family: Poppins;
  font-size: 10px;
  font-style: normal;
  font-weight: 700;
  line-height: normal;
  padding: 4px 6px;
  color: ${leo.color.neutral[50]};
  text-transform: uppercase;
  cursor: pointer;
`

export const ReviewButtonRow = styled(Row)`
  max-width: 360px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    max-width: unset;
  }
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const CaratIcon = styled(Icon).attrs({
  name: 'carat-right'
})`
  --leo-icon-size: 24px;
  color: inherit;
  margin-left: 8px;
`

export const ButtonText = styled(Text)`
  overflow: hidden;
  color: inherit;
  white-space: pre-wrap;
  word-break: break-all;
  font-weight: 500;
`

export const Button = styled(WalletButton)<{
  isPlaceholder: boolean
}>`
  cursor: pointer;
  display: flex;
  outline: none;
  border: none;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  background-color: transparent;
  padding: 10px 0px;
  color: ${(p) =>
    p.isPlaceholder ? leo.color.text.tertiary : leo.color.text.primary};
  white-space: nowrap;
  :disabled {
    cursor: not-allowed;
  }
  &:hover:not([disabled]) {
    color: ${leo.color.text.interactive};
  }
`

export const AlertMessage = styled(Alert)`
  --leo-alert-center-width: 100%;
  margin-bottom: 16px;
`

export const AlertMessageButton = styled(LeoButton)`
  --leo-button-padding: 0px;
`

export const AlertMessageWrapper = styled(Row)`
  flex-wrap: wrap;
`
