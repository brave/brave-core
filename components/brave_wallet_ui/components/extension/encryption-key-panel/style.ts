// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  orb: string
  needsCenterAlignment: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${(p) => p.theme.color.background01};
`

export const TopRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 15px 15px 0px 15px;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 54px;
  height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-bottom: 13px;
`

export const AccountNameText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  line-height: 20px;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 2px;
`

export const NetworkText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const PanelTitle = styled.span`
  width: 80%;
  font-family: Poppins;
  font-size: 18px;
  line-height: 26px;
  letter-spacing: 0.02em;
  text-align: center;
  color: ${(p) => p.theme.color.text01};
  font-weight: 600;
  margin-bottom: 6px;
`

export const MessageBox = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: ${(p) => p.needsCenterAlignment ? 'center' : 'flex-start'};
  justify-content: ${(p) => p.needsCenterAlignment ? 'center' : 'flex-start'};
  flex-direction: column;
  border: 1px solid ${(p) => p.theme.color.divider01};
  box-sizing: border-box;
  border-radius: 4px;
  width: 255px;
  height: 140px;
  padding: 8px 14px;
  margin-bottom: 14px;
  overflow-x: hidden;
  overflow-y: scroll;
`

export const MessageText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: left;
  color: ${(p) => p.theme.color.text02};
  word-break: break-word;
`

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
`

export const DecryptButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  font-weight: 600;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  background: none;
  color: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.palette.blurple300};
  }
`
