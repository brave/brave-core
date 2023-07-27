// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'
import { Row, WalletButton } from '../../../shared/style'

export const StyledWrapper = styled.div<{ isScrolled: boolean }>`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  top: 48px;
  left: 0;
  right: 0;
  position: fixed;
  z-index: 8;
  box-sizing: border-box;
  opacity: ${(p) => (p.isScrolled ? 0 : 1)};
  transition-duration: 0.4s;
  transition-timing-function: ease-out;
  transition-delay: 0s;
`

export const TitleWrapper = styled(Row)<{
  isReadyToConnect: boolean
  isScrolled: boolean
}>`
  position: relative;
  z-index: 20;
  background-color: ${leo.color.container.background};
  box-shadow: ${(p) =>
    p.isScrolled ? '0px 3px 10px 1px rgba(0, 0, 0, 0.08)' : 'none'};
  transition-duration: 0.4s;
  transition-timing-function: ease-out;
  transition-delay: 0s;
  @media (prefers-color-scheme: dark) {
    background-color: ${(p) =>
      p.isReadyToConnect
        ? leo.color.container.background
        : leo.color.container.highlight};
  }
`

export const Title = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
  text-align: center;
  color: ${leo.color.text.primary};
`

export const SiteName = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 16px;
  line-height: 28px;
  text-align: left;
  color: ${leo.color.text.primary};
`

export const SiteURL = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  text-align: left;
  color: ${leo.color.text.secondary};
  word-break: break-word;
`

export const FavIcon = styled.img<{ isReadyToConnect: boolean }>`
  width: ${(p) => (p.isReadyToConnect ? 40 : 48)}px;
  height: ${(p) => (p.isReadyToConnect ? 40 : 48)}px;
  margin-right: ${(p) => (p.isReadyToConnect ? 0 : 16)}px;
  border-radius: 8px;
`

export const MessageBox = styled(Row)`
  background-color: ${leo.color.systemfeedback.infoBackground};
  border-radius: 8px;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const InfoIcon = styled(Icon)`
  --leo-icon-size: 20px;
  color: ${leo.color.systemfeedback.infoIcon};
  margin-right: 16px;
`

export const BackButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  margin: 0px;
  padding: 0px;
`

export const BackIcon = styled(Icon)`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const GradientLine = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  width: 80px;
  height: 2px;
  background: linear-gradient(
    90deg,
    rgba(0, 0, 0, 0) 0%,
    ${leo.color.systemfeedback.successIcon} 50%,
    rgba(0, 0, 0, 0) 100%
  );
  margin: 0px 4px;
`

export const LinkIconCircle = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  position: absolute;
  width: 24px;
  height: 24px;
  background: ${leo.color.systemfeedback.successIcon};
  border-radius: 24px;
`

export const LinkIcon = styled(Icon)`
  --leo-icon-size: 16px;
  color: ${leo.color.white};
`
