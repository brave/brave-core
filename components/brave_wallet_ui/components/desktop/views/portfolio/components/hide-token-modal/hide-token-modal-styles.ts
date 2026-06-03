// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import { WalletButton } from '../../../../../shared/style'

export const hideTokenModalWidth = '400px'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  padding: 0 20px 20px 20px;
`

export const TokenSymbol = styled.h6`
  font: ${leo.font.large.regular};
  margin: 0;
  padding-top: 20px;
  padding-bottom: 7px;
  color: ${leo.color.text.secondary};
  display: flex;
  align-self: center;
`

export const Instructions = styled.p`
  font: ${leo.font.large.regular};
  margin: 0;
  padding-top: 20px;
  padding-bottom: 7px;
  color: ${leo.color.text.secondary};
  text-align: center;
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin-top: 15px;
`

export const OkButton = styled(WalletButton)`
  font: ${leo.font.default.regular};

  background: ${leo.color.blurple[50]};
  border: 1px solid ${leo.color.blurple[50]};
  height: 40px;
  width: 125px;
  margin-top: 70px;
  border-radius: 100px;
  display: flex;
  align-self: center;
  justify-content: center;
  align-items: center;
  color: ${leo.color.white};
  cursor: pointer;
`

export const CancelButton = styled(WalletButton)`
  font: ${leo.font.default.regular};

  background: none;
  border: 1px solid ${leo.color.blurple[50]};
  height: 40px;
  width: 125px;
  margin-top: 70px;
  border-radius: 100px;
  display: flex;
  align-self: center;
  justify-content: center;
  align-items: center;
  color: ${leo.color.blurple[50]};
  cursor: pointer;
`

export const IconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
`
