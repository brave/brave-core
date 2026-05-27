// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const InstructionBox = styled.div`
  width: 100%;
  border: 1px solid ${(p) => p.theme.color.divider01};
  border-radius: 4px;
  padding: 10px 10px 0px 10px;
  box-sizing: border-box;
  margin-top: 6px;
`

export const InstructionParamBox = styled.div`
  width: 100%;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  gap: 6px;
  margin-bottom: 12px;
  margin-top: 12px;
  word-break: break-all;
  font: ${leo.font.small.regular};
color: ${(p) => p.theme.color.text01};
  text-align: left;

  & > * {
    width: 100%;
  }

  & > var {
    text-align: left;
    display: block;
    font: ${leo.font.small.regular};
color: ${(p) => p.theme.color.text02};
    font: ${leo.font.small.regular};
margin-left: 4px;
  }
`

export const AddressText = styled.span<{ isBold?: true }>`
  color: ${(p) => p.theme.color.text02};
  font: ${leo.font.small.regular};
margin-left: 4px;
  text-align: left;
  display: block;
`

export const CodeSectionTitle = styled.span`
  color: ${(p) => p.theme.color.text02};
  font: ${leo.font.small.regular};
margin-left: 4px;
  margin-bottom: 4px;
  text-align: left;
  display: block;
`
