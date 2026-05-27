// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

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

export const InputLabel = styled.span`
  font: ${leo.font.default.regular};
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 6px;
`

export const Input = styled.input`
  box-sizing: border-box;
  outline: none;
  width: 100%;
  background-image: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
  font-style: normal;
  font: ${leo.font.small.regular};
  padding: 10px;
  margin-bottom: 8px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font: ${leo.font.small.regular};
    color: ${(p) => p.theme.color.text03};
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

export const InfoText = styled.span`
  font: ${leo.font.small.regular};
  color: ${(p) => p.theme.color.text03};
  word-break: break-word;
  margin-bottom: 12px;
`
