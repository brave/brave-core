// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const InstructionBox = styled.div`
  width: 100%;
  border: 1px solid ${(p) => p.theme.color.divider01};
  border-radius: 4px;
  padding: 8px;
`

export const InstructionParamBox = styled.div`
  width: 100%;

  display: flex;
  flex-direction: column;
  gap: 8px;
  
  margin-bottom: 8px;

  word-break: break-all;

  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text01};
  text-align: left;

  & > * {
    font-family: Poppins;
    font-style: normal;
    width: 100%;
  } 

  & > var {
    margin-top: 4px;
    font-weight: 600;
    text-align: left;
    display: block;
  }
  
  & > samp {
    font-weight: 400;
  }
`
