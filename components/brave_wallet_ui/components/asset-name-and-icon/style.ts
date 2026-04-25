// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { layoutPanelWidth } from '../desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  height: auto;
  align-items: center;
`

export const AssetIcon = styled.img`
  width: 40px;
  height: auto;
  margin-right: 16px;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    width: 32px;
    margin-right: 12px;
  }
`

export const NameAndSymbolWrapper = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  align-items: flex-start;
  height: 100%;
  padding: 1px 0;
`

export const AssetName = styled.span`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 600;
  line-height: 24px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  width: 160px;
  text-align: left;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    width: 140px;
  }
`

export const AssetSymbol = styled.span`
  color: ${leo.color.text.tertiary};
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 400;
  line-height: 18px;
  text-transform: uppercase;
  text-align: left;
`
