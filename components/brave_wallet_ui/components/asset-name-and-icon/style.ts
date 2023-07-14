// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  height: 40px;

`

export const AssetIcon = styled.img`
  width: 40px;
  height: auto;
  margin-right: 12px;
`

export const NameAndSymbolWrapper = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  height: 100%;
  padding: 1px 0;
`

export const AssetName = styled.span`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 700;
  line-height: 20px;
  letter-spacing: 0.28px;
`

export const AssetSymbol = styled.span`
  color: ${leo.color.legacy.text2};
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 500;
  line-height: 18px;
  letter-spacing: 0.24px;
  text-transform: uppercase;
`
