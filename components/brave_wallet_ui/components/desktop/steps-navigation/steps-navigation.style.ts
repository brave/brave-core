// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import { WalletButton } from '../../shared/style'

const Font = css`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  color: ${(p) => p.theme.color.text02};
`

export const Wrapper = styled.div`
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  margin-bottom: 40px;
`

export const BackButton = styled(WalletButton)<{
  to?: string // used for links
}>`
  ${Font}
  display: flex;
  flex: 1;
  flex-direction: row;
  align-items: center;
  text-align: center;
  align-self: flex-start;
  color: ${(p) => p.theme.color.text01};
  cursor: pointer;
  outline: none;
  background: none;
  padding: 3px 0px;
  line-height: 20px;
  letter-spacing: 0.01em;
  border: none;
  text-decoration: ${(p) => p.to ? 'none' : 'unset'};

  & > span {
    margin-left: 8px;
  }
`

export const DotsWrapper = styled.div`
  display: flex;
  flex: 7;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  align-self: center;
  
  & > * {
    display: inline;
    width: 10px;
    height: 10px;
    border-radius: 25px;
    margin-right: 16px;
    background-color: ${(p) => p.theme.color.divider01};
  }

  & > .active {
    background-color: ${(p) => p.theme.color.interactive05};
  }
`

export const FlexBox = styled.div`
  flex: 1;
`
