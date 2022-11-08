// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

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

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 8px;
`

export const Description = styled.span<StyleProps>`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 14px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 520px;
  text-align: ${(p) => p.textAlign ? p.textAlign : 'center'};
  margin-bottom: 25px;
`

export const FormText = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
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

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
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
  width: 100%;
  outline: none;
  background-image: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 10px;
  margin-bottom: 10px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: 12px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text03};
    font-weight: normal;
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
