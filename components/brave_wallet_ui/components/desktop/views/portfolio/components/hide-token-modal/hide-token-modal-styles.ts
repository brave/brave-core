// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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
  font-family: 'Poppins';
  font-style: normal;
  font-size: 16px;
  line-height: 1.4;
  font-weight: 400;
  margin: 0;
  padding-top: 20px;
  padding-bottom: 7px;
  color: ${p => p.theme.color.text02};
  display: flex;
  align-self: center;
`

export const Instructions = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-size: 16px;
  line-height: 1.4;
  font-weight: 400;
  margin: 0;
  padding-top: 20px;
  padding-bottom: 7px;
  color: ${p => p.theme.color.text02};
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
  background: ${p => p.theme.palette.blurple500};
  border: 1px solid ${p => p.theme.palette.blurple500};
  height: 40px;
  width: 125px;
  margin-top: 70px;
  border-radius: 100px;
  font-family: 'Poppins';
  font-style: normal;
  font-size: 14px;
  font-weight: 400;
  display: flex;
  align-self: center;
  justify-content: center;
  align-items: center;
  color: ${p => p.theme.palette.white};
  cursor: pointer;
`

export const CancelButton = styled(WalletButton)`
  background: none;
  border: 1px solid ${p => p.theme.palette.blurple500};
  height: 40px;
  width: 125px;
  margin-top: 70px;
  border-radius: 100px;
  font-family: 'Poppins';
  font-style: normal;
  font-size: 14px;
  font-weight: 400;
  display: flex;
  align-self: center;
  justify-content: center;
  align-items: center;
  color: ${p => p.theme.palette.blurple500};
  cursor: pointer;
`

export const IconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
`
