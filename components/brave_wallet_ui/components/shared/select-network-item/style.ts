// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../style'
import CheckMark from '../../../assets/svg-icons/big-checkmark.svg'

export const StyledWrapper = styled(WalletButton)`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin: 10px 0px;
  padding: 0px;
`

export const LeftSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  margin-right: 6px;
`

export const NetworkName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  text-align: left;
`

export const BigCheckMark = styled.div`
  width: 14px;
  height: 14px;
  background-color: ${(p) => p.theme.color.text01};
  -webkit-mask-image: url(${CheckMark});
  mask-image: url(${CheckMark});
  margin-right: 8px;
`
