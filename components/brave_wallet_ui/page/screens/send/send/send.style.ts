// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Assets
import { LoaderIcon } from 'brave-ui/components/icons'

// Shared Styles
import { StyledDiv, StyledInput, Row } from '../shared.styles'

export const sendContainerWidth = 512

export const SendContainer = styled(StyledDiv)`
  box-sizing: border-box;
  justify-content: flex-start;
  width: 100%;
  position: relative;
`

export const SectionBox = styled(StyledDiv) <{
  hasError?: boolean
  hasWarning?: boolean
  minHeight?: number
  noPadding?: boolean
  boxDirection?: 'row' | 'column'
}>`
  background-color: ${(p) => p.theme.color.background02};
  flex-direction: ${(p) => p.boxDirection ? p.boxDirection : 'column'};
  box-sizing: border-box;
  border-radius: 16px;
  border: 1px solid
    ${(p) => (p.hasError ? p.theme.color.errorBorder : p.hasWarning ? p.theme.color.warningBorder : p.theme.color.divider01)};
  padding: ${(p) => p.noPadding ? '0px' : '16px 16px 16px 8px'};
  width: 100%;
  position: relative;
  margin-bottom: 16px;
  min-height: ${(p) => (p.minHeight ? `${p.minHeight}px` : 'unset')};
`

export const AmountInput = styled(StyledInput) <{
  hasError: boolean
}>`
  color: ${(p) => (p.hasError ? p.theme.color.errorBorder : 'inherit')};
  font-weight: 500;
  font-size: 28px;
  line-height: 42px;
  text-align: right;
  width: 100%;
  ::placeholder {
    color: ${(p) => p.theme.color.text03};
  }
`

export const AddressInput = styled(StyledInput) <{
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
  overflow:auto;
  font-family: 'Poppins';
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
`

export const InputRow = styled(Row)`
  box-sizing: border-box;
  position: relative;
`

export const DomainLoadIcon = styled(LoaderIcon) <{ position: number }>`
  color: ${p => p.theme.palette.blurple500};
  height: 16px;
  width: 16px;
  opacity: 0.4;
  position: absolute;
  z-index: 8;
  left: ${(p) => p.position}px;
`
