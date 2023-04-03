// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const InstructionBox = styled.div`
  width: 100%;
  border: 1px solid ${(p) => p.theme.color.divider01};
  border-radius: 4px;
  padding: 10px 10px 0px 10px;
  box-sizing: border-box;
  margin-top: 6px;
`

export const InstructionParamBox = styled.div`
  font-family: Poppins;
  width: 100%;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  gap: 6px;
  margin-bottom: 12px;
  margin-top: 12px;
  word-break: break-all;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text01};
  text-align: left;

  & > * {
    font-style: normal;
    width: 100%;
  } 

  & > var {
    font-weight: 600;
    text-align: left;
    display: block;
    font-size: 12px;
  }
  
  & > samp {
    font-weight: 400;
    color: ${(p) => p.theme.color.text02};
    font-size: 12px;
    margin-left: 4px;
  }
`

export const AddressText = styled.span<
  { isBold?: true }
>`
  font-family: Poppins;
  font-weight: ${(p) => p.isBold ? 600 : 400};
  color: ${(p) => p.theme.color.text02};
  font-size: 12px;
  margin-left: 4px;
  text-align: left;
  display: block;
`

export const CodeSectionTitle = styled.span`
  font-family: Poppins;
  font-weight: 400;
  color: ${(p) => p.theme.color.text02};
  font-size: 12px;
  margin-left: 4px;
  margin-bottom: 4px;
  text-align: left;
  display: block;
`
