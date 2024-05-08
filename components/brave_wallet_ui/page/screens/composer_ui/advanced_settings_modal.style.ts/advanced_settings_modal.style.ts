// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Radio from '@brave/leo/react/radioButton'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Column, Row, Text } from '../../../../components/shared/style'
import {
  layoutSmallWidth //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const SlippageButton = styled(Button)<{
  isSelected: boolean
}>`
  --leo-button-color: ${(p) =>
    p.isSelected ? leo.color.button.background : leo.color.text.secondary};
  margin-right: 18px;
`

export const SectionWrapper = styled(Column)`
  padding: 24px 40px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 16px;
  }
`

export const InputWrapper = styled(Row)`
  display: flex;
  background-color: ${leo.color.container.highlight};
  border: 1px solid ${leo.color.container.highlight};
  border-radius: 8px;
  flex-direction: row;
  padding: 10px 12px;
  justify-content: space-between;
  box-sizing: border-box;
`

export const CustomSlippageInput = styled.input`
  height: 100%;
  width: 100%;
  border: none;
  background: none;
  background-color: none;
  color: ${leo.color.text.primary};
  text-align: right;
  font-family: 'Poppins';
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 22px;
  letter-spacing: -0.1px;
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

export const PercentText = styled(Text)`
  color: ${leo.color.text.tertiary};
  line-height: 22px;
`

export const RadioButton = styled(Radio)`
  width: 100%;
  margin-bottom: 16px;
`

export const DarkText = styled(Text)`
  line-height: 22px;
  color: ${leo.color.text.primary};
`

export const LightText = styled(DarkText)`
  color: ${leo.color.text.tertiary};
`

export const ButtonRow = styled(Row)`
  padding: 0px 40px 24px 40px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 0px 16px 16px 16px;
  }
`

export const InfoIcon = styled(Icon).attrs({
  name: 'info-outline'
})`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`

export const TooltipContent = styled.div`
  max-width: 280px;
`
