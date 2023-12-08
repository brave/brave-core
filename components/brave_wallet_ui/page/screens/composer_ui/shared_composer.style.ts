// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

// Shared Styles
import { Column, WalletButton } from '../../../components/shared/style'
import {
  layoutPanelWidth //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const ToSectionWrapper = styled(Column)`
  padding: 0px 32px 32px 32px;
  border-radius: 0px 0px 24px 24px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    border-radius: 0px;
    padding: 0px 0px 16px 0px;
    height: 100%;
  }
  background-color: ${leo.color.container.interactive};
  @media (prefers-color-scheme: dark) {
    background-color: ${leo.color.container.highlight};
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
  background-color: ${leo.color.gray[10]};
  border-radius: 4px;
  font-family: Poppins;
  font-size: 10px;
  font-style: normal;
  font-weight: 700;
  line-height: normal;
  padding: 4px 6px;
  color: ${leo.color.gray[50]};
  text-transform: uppercase;
  cursor: pointer;
`
