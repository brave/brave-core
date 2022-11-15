// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
`

export const StatWrapper = styled.div`
  display: flex;
  flex-direction: column;
  margin-right: 95px;
 `

export const StatValue = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 24px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text01};
  margin-bottom: 12px;
  justify-content: center;
`

export const StatLabel = styled.div`
 display: flex;
 align-items: center;
 text-align: center;
 font-family: 'Poppins';
 font-style: normal;
 font-weight: 400;
 font-size: 14px;
 line-height: 20px;
 letter-spacing: 0.01em;
 color: ${p => p.theme.color.text02};
 justify-content: center;
`

export const Row = styled.div`
  display: flex;
  flex-direction: row;
  margin: 16px 0 16px 0;
`

export const Currency = styled.sup`
 display: flex;
 font-family: 'Poppins';
 font-style: normal;
 font-weight: 500;
 font-size: 12px;
 line-height: 18px;
 letter-spacing: 0.01em;
 color: ${p => p.theme.color.text02};
 top: -0.8em;
 margin-right: 2px;
`
