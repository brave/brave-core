// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import {
  StyledInput,
  Icon,
  StyledDiv,
  StyledLabel
} from '../../shared-swap.styles'

export const HiddenCheckBox = styled(StyledInput)`
  border: 0;
  clip: rect(0 0 0 0);
  height: 1px;
  margin: -1px;
  overflow: hidden;
  padding: 0;
  position: absolute;
  white-space: nowrap;
  width: 1px;
`

export const StyledIcon = styled(Icon)`
  color: ${(p) => p.theme.palette.white};
`

export const StyledCheckbox = styled(StyledDiv)<{ isChecked: boolean }>`
  width: 20px;
  height: 20px;
  background: ${(p) =>
    p.isChecked ? p.theme.color.interactive05 : p.theme.color.background01};
  border-radius: 4px;
  box-shadow: ${(p) =>
    p.isChecked
      ? 'none'
      : `inset 0px 0px 0px 1px ${p.theme.color.interactive08}`};
`

export const Label = styled(StyledLabel)<{
  isChecked: boolean
  labelSize?: '12px' | '14px'
  isBold?: boolean
}>`
  display: flex;
  flex-direction: row;
  gap: 12px;
  cursor: pointer;
  font-weight: ${(p) => (p.isBold ? 400 : 200)};
  font-size: ${(p) => (p.labelSize ? p.labelSize : '12px')};
  color: ${(p) => p.theme.color.text02};
`
