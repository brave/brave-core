// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const InstructionParamBox = styled.div`
  display: flex;
  flex-direction: column;
  flex-wrap: wrap;
  align-items: flex-start;
  justify-content: flex-start;
  
  width: 100%;
  margin-bottom: 8px;

  word-break: break-all;

  width: 100%;
  display: block;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text01};
  text-align: left;

  & > var {
    font-weight: 600;
    font-size: 14px;
    text-align: left;
    display: block;
  }
  
  & > samp {
    font-weight: 400;
    text-align: right;
  }
`
