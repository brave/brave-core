// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

// Assets
import { LoaderIcon } from 'brave-ui/components/icons'

// Shared Styles
import { StyledInput } from '../shared.styles'
import { Text, Row } from '../../../../components/shared/style'

export const AddressInput = styled(StyledInput)<{
  hasError: boolean
}>`
  color: ${(p) => (p.hasError ? p.theme.color.errorBorder : 'inherit')};
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
  width: 100%;
  z-index: 9;
  position: relative;
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
  }
`

export const DIVForWidth = styled.div`
  width: auto;
  display: inline-block;
  visibility: hidden;
  position: fixed;
  overflow: auto;
  font-family: 'Poppins';
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
`

export const InputRow = styled(Row)`
  box-sizing: border-box;
  position: relative;
`

export const DomainLoadIcon = styled(LoaderIcon)<{ position: number }>`
  color: ${(p) => p.theme.palette.blurple500};
  height: 16px;
  width: 16px;
  opacity: 0.4;
  position: absolute;
  z-index: 8;
  left: ${(p) => p.position}px;
`

export const ToText = styled(Text)`
  line-height: 26px;
  color: ${leo.color.text.tertiary};
`

export const ToRow = styled(Row)`
  min-height: 26px;
`
