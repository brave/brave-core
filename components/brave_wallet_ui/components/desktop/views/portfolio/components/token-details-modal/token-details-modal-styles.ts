// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import { WalletButton } from '../../../../../shared/style'
export const modalWidth = '500px'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  padding: 0 20px 20px 20px;
`

export const TokenBalanceRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
`
export const CryptoBalance = styled.p`
  font: ${leo.font.heading.display2};
margin: 0;
  padding: 0 16px 0 0;
  color: ${(p) => p.theme.color.text01};
`

export const FiatBalance = styled.h6`
  font: ${leo.font.large.regular};
margin: 0;
  padding-top: 20px;
  padding-bottom: 4px;
  color: ${(p) => p.theme.color.text02};
`

export const DetailColumn = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  margin-top: 18px;
`

export const TokenDetailLabel = styled.h6`
  font: ${leo.font.default.semibold};
margin: 0;
  padding: 0;
  color: ${(p) => p.theme.color.text01};
`

export const TokenDetailValue = styled.h6`
  font: ${leo.font.default.regular};
margin: 8px 0 0 0;
  padding: 0;
  color: ${(p) => p.theme.color.text01};
`

export const ContractAddress = styled(TokenDetailValue)`
  cursor: pointer;
`

export const HideTokenButton = styled(WalletButton)`
  background: none;
  border: 1px solid ${(p) => p.theme.palette.blurple500};
  height: 40px;
  margin-top: 70px;
  border-radius: 100px;
  font: ${leo.font.default.regular};
width: 320px;
  display: flex;
  align-self: center;
  justify-content: center;
  align-items: center;
  color: ${(p) => p.theme.palette.blurple500};
  cursor: pointer;
`
