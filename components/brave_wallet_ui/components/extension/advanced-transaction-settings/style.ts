// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

// Shared Styles
import { Text } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: column;
  width: 100%;
  height: 100%;
  padding: 5px 15px 15px 15px;
`

export const FormColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  width: 100%;
  align-self: center;
`

export const InputLabel = styled(Text)`
  margin-bottom: 6px;
`

export const Input = styled.input`
  font: ${leo.font.default.regular};

  box-sizing: border-box;
  outline: none;
  width: 100%;
  background-image: none;
  background-color: ${leo.color.container.background};
  box-shadow: none;
  border: 1px solid ${leo.color.neutral[30]};
  border-radius: 4px;
  letter-spacing: 0.01em;
  padding: 10px;
  margin-bottom: 8px;
  color: ${leo.color.text.primary};
  ::placeholder {
    letter-spacing: 0.01em;
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

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  flex-wrap: wrap-reverse;
  gap: 8px;
`

export const InfoText = styled(Text)`
  word-break: break-word;
  margin-bottom: 12px;
`
