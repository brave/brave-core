// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

// Shared Styles
import { Text } from '../../../components/shared/style'

interface StyleProps {
  textAlign?: 'left' | 'right' | 'center' | 'justify'
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  padding-top: 50px;
`

export const Title = styled(Text)`
  margin-bottom: 8px;
`

export const Description = styled(Text)<StyleProps>`
  display: flex;
  align-items: center;
  font-weight: 300;
  max-width: 520px;
  text-align: ${(p) => (p.textAlign ? p.textAlign : 'center')};
  margin-bottom: 25px;
`

export const FormText = styled(Text)`
  font-size: 15px;
  margin-bottom: 8px;
`

export const FormWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  max-width: 550px;
`

export const ErrorText = styled(Text)`
  margin-bottom: 10px;
`

export const LegacyCheckboxRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const CheckboxRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  margin-bottom: 50px;
`

export const RecoveryPhraseInput = styled.input`
  font: ${leo.font.components.tableheader};
  width: 100%;
  outline: none;
  background-image: none;
  background-color: ${leo.color.container.background};
  box-shadow: none;
  border: 1px solid ${leo.color.neutral[30]};
  border-radius: 4px;
  letter-spacing: 0.01em;
  padding: 10px;
  margin-bottom: 10px;
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

export const InputColumn = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 250px;
  margin-bottom: 28px;
`
