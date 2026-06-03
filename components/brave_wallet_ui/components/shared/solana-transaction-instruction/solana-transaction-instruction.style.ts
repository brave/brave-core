// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

// Shared Styles
import { Text } from '../style'

export const InstructionBox = styled.div`
  width: 100%;
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 4px;
  padding: 10px 10px 0px 10px;
  box-sizing: border-box;
  margin-top: 6px;
`

export const InstructionParamBox = styled.div`
  font: ${leo.font.small.regular};

  width: 100%;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  gap: 6px;
  margin-bottom: 12px;
  margin-top: 12px;
  word-break: break-all;
  letter-spacing: 0.01em;
  color: ${leo.color.text.primary};
  text-align: left;

  & > * {
    width: 100%;
  }

  & > var {
    text-align: left;
    display: block;
  }

  & > samp {
    font-weight: 400;
    color: ${leo.color.text.secondary};
    font-size: 12px;
    margin-left: 4px;
  }
`

export const AddressText = styled(Text)`
  margin-left: 4px;
  display: block;
`

export const CodeSectionTitle = styled(Text)`
  margin-left: 4px;
  margin-bottom: 4px;
  display: block;
`
