// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../../../../../shared/style'

export const StyledWrapper = styled.div`
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  grid-gap: 25px;
  box-sizing: border-box;
  width: 100%;
  padding-top: 10px;
  @media screen and (max-width: 1350px) {
    grid-template-columns: repeat(4, 1fr);
  }
  @media screen and (max-width: 1150px) {
    grid-template-columns: repeat(3, 1fr);
  }
  @media screen and (max-width: 950px) {
    grid-template-columns: repeat(2, 1fr);
  }
`

export const NFTButton = styled(WalletButton)`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  box-sizing: border-box;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin: 0px;
  padding: 0px;
  &:last-child {
    margin-right: 0px;
  }
`

export const IconWrapper = styled.div`
  position: relative;
  overflow: hidden;
  width: 100%;
  padding-top: 100%;
`

export const DIVForClickableArea = styled.div`
  display: block;
  position: absolute;
  z-index: 4;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  width: 100%;
  height: 100%;
`

export const NFTText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  margin-top: 6px;
`
