// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
`

export const HeaderRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  padding: 4px 12px 12px ;
`

export const HeaderColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
`

export const AddressContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  height: 100%;
`

export const AddressScrollContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  max-height: 240px;
  padding-top: 8px;
  padding-left: 10px;
  padding-right: 10px;
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;
  box-sizing: border-box;
`

export const SiteOriginTitle = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 2px;
  margin-right: 10px;
  word-break: break-word;
`

export const AccountsTitle = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  margin-top: 4px;
`

export const FavIcon = styled.img`
  width: 48px;
  height: 48px;
  border-radius: 5px;
  background-color: ${(p) => p.theme.palette.grey200};
  margin-left: 12px;
  margin-right: 12px;
`

export const NewAccountButton = styled(WalletButton)`
  display: flex;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  font-weight: 600;
  margin-bottom: 8px;
  margin-left: 12px;
  color: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.palette.blurple300};
  }
  letter-spacing: 0.01em;
`
