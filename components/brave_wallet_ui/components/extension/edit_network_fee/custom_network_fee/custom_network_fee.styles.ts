// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Row, Text } from '../../../shared/style'

export const StyledWrapper = styled(Column)`
  overflow: hidden;
`

export const Card = styled(Column)`
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.xl};
`

export const Description = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const SectionLabel = styled(Text)`
  font: ${leo.font.small.semibold};
  letter-spacing: ${leo.typography.letterSpacing.small};
`

export const InputWrapper = styled(Row)<{
  hasError?: boolean
}>`
  cursor: pointer;
  background-color: ${leo.color.container.background};
  outline: 1px solid
    ${(p) =>
      p.hasError
        ? leo.color.systemfeedback.errorVibrant
        : leo.color.divider.subtle};
  transition:
    outline 0.1s ease-in-out,
    box-shadow 0.1s ease-in-out;
  border-radius: ${leo.radius.m};
  :hover {
    outline: 1px solid ${leo.color.divider.strong};
    box-shadow: ${leo.effect.elevation['02']};
  }
  :focus-within {
    outline: 2px solid
      ${(p) =>
        p.hasError
          ? leo.color.systemfeedback.errorVibrant
          : leo.color.primary[40]};
  }
`

export const Input = styled.input`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
  background-color: ${leo.color.container.background};
  color: ${leo.color.text.primary};
  outline: none;
  border: none;
  width: 100%;
  padding: 0px;
  text-align: right;
  ::placeholder {
    font: ${leo.font.small.regular};
    letter-spacing: ${leo.typography.letterSpacing.small};
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

export const InputLabel = styled(Text)`
  font: ${leo.font.small.regular};
  letter-spacing: ${leo.typography.letterSpacing.small};
`
