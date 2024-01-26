// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import LeoIcon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton, Text } from '../../shared/style'

export const Input = styled.input<{ hasError: boolean }>`
  box-sizing: border-box;
  width: 100%;
  background-image: none;
  box-shadow: none;
  background-color: ${leo.color.container.highlight};
  border: ${(p) =>
    p.hasError
      ? `2px solid ${leo.color.systemfeedback.errorIcon}`
      : `2px solid transparent`};
  padding: 10px 8px 10px 16px;
  border-radius: 8px;
  font-family: Poppins;
  font-style: normal;
  font-size: 14px;
  line-height: 24px;
  font-weight: 400;
  margin: 0px;
  color: ${leo.color.text.primary};

  ::placeholder {
    color: ${leo.color.text.tertiary};
  }

  :focus, :focus-visible {
    outline: none;
    border: ${(p) =>
      p.hasError
        ? `2px solid ${leo.color.systemfeedback.errorIcon}`
        : `2px solid ${leo.color.text.interactive}`};
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

export const InputLabel = styled.label`
  font-family: 'Inter Variable', 'Poppins';
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
  color: ${leo.color.text.primary};
  margin-bottom: 4px;
`

export const Asterisk = styled.span`
  color: ${leo.color.systemfeedback.errorIcon};
  margin-left: 4px;
`

export const ErrorText = styled(Text)`
  line-height: 18px;
  font-weight: 400;
  color: ${leo.color.systemfeedback.errorIcon};
`

export const ErrorIcon = styled(LeoIcon).attrs({
  name: 'warning-triangle-outline'
})`
  --leo-icon-size: 16px;
  color: ${leo.color.systemfeedback.errorIcon};
  margin-right: 8px;
`

export const ToggleVisibilityButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  right: 10px;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  width: 20px;
  height: 20px;
`

export const ToggleVisibilityIcon = styled(LeoIcon)`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`
