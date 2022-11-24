// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { WalletButton } from '../../shared/style'
interface StyleProps {
  buttonType: 'primary' | 'secondary'
  bannerType: 'warning' | 'danger'
}

export const StyledWrapper = styled.div<Partial<StyleProps>>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  background-color: ${(p) => p.bannerType === 'warning' ? p.theme.color.warningBackground : p.theme.color.errorBackground};
  border-radius: 4px;
  border: 1px solid ${(p) => p.bannerType === 'warning' ? p.theme.color.warningBorder : p.theme.color.errorBorder};
  padding: 20px;
  margin-bottom: 14px;
  @media screen and (max-width: 1080px) {
    flex-direction: column;
    align-items: flex-start;
  }
 `

export const WarningText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  font-weight: 500;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
  @media screen and (max-width: 1080px) {
    margin-bottom: 12px;
  }
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const BannerButton = styled(WalletButton) <Partial<StyleProps>>`
  display: flex;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
  font-family: Poppins;
  font-size: 12px;
  font-weight: 600;
  color: ${(p) => p.buttonType === 'primary' ? p.theme.color.interactive05 : p.theme.color.text02};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.buttonType === 'primary' ? p.theme.palette.white : p.theme.color.text02};
  }
  letter-spacing: 0.01em;
  margin-left: 20px;
  @media screen and (max-width: 1080px) {
    margin-left: 0px;
    margin-right: 20px;
  }
`
