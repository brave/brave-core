// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import { WalletButton, Text } from '../../shared/style'

export const SettingsButton = styled(WalletButton)<{
  showConnectionStatus: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  cursor: pointer;
  border-radius: 12px;
  outline: none;
  border: 1px solid ${leo.color.divider.subtle};
  background: none;
  padding: ${(p) => (p.showConnectionStatus ? '8px 16px 8px 12px' : '8px')};
  gap: 8px;
  margin-right: 16px;
  background-color: transparent;
  position: relative;
  z-index: 10;
`

export const ConnectedIcon = styled(Icon)<{
  size: string
  dappConnected?: boolean
}>`
  --leo-icon-size: ${(p) => p.size};
  color: ${(p) =>
    p.dappConnected
      ? leo.color.systemfeedback.successIcon
      : leo.color.icon.default};
`

export const ConnectedText = styled(Text)<{
  dappConnected?: boolean
}>`
  color: ${(p) =>
    p.dappConnected
      ? leo.color.systemfeedback.successIcon
      : leo.color.icon.default};
  line-height: 24px;
`

export const OverlapForClick = styled.div`
  display: flex;
  position: fixed;
  top: 0px;
  left: 0px;
  right: 0px;
  bottom: 0px;
  z-index: 11;
`

export const SettingsBubbleWrapper = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: flex-start;
  flex-direction: column;
  position: fixed;
  top: 60px;
  left: 0px;
  right: 0px;
  bottom: 0px;
  z-index: 20;
  padding: 0px 8px;
`

export const BackgroundBlur = styled.div`
  position: fixed;
  top: 70px;
  left: 0px;
  right: 0px;
  bottom: 0px;
  z-index: 19;
  background: rgba(33, 37, 41, 0.32);
  backdrop-filter: blur(8px);
`

export const SettingsBubble = styled.div`
  position: absolute;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  border-radius: 16px;
  background-color: ${leo.color.container.background};
  box-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.1);
  top: 8px;
  left: 8px;
  right: 8px;
  z-index: 21;
  padding: 16px;
  max-height: 480px;
`

export const Pointer = styled.div`
  position: absolute;
  width: 0;
  height: 0;
  border-style: solid;
  border-width: 0 7px 8px 7px;
  border-color: transparent transparent ${leo.color.container.background}
    transparent;
  margin-right: 85px;
  z-index: 22;
`

export const PointerShadow = styled(Pointer)`
  filter: drop-shadow(0px 0px 4px rgba(0, 0, 0, 0.1));
  z-index: 20;
`

export const SectionRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  border-radius: 8px;
  background-color: ${leo.color.container.highlight};
  padding: 12px;
  width: 100%;
`

export const SectionButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  cursor: pointer;
  border-radius: 8px;
  outline: none;
  background: none;
  border: none;
  background-color: ${leo.color.container.highlight};
  padding: 8px 12px;
  width: 100%;
`

export const ButtonIcon = styled(Icon).attrs({
  name: 'carat-right'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const TitleText = styled(Text)`
  color: ${leo.color.text.primary};
  font-weight: 500;
  line-height: 32px;
`

export const NameText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 24px;
  word-break: break-all;
  text-align: left;
`

export const NetworkName = styled.span`
  font-family: 'Poppins';
  color: ${leo.color.text.primary};
  line-height: 24px;
  word-wrap: wrap;
  word-break: break-all;
  text-align: left;
  font-size: 14px;
  font-weight: 400;
  letter-spacing: 0.02em;
`

export const DescriptionText = styled(Text)`
  color: ${leo.color.text.secondary};
  line-height: 18px;
  word-break: break-all;
  text-align: left;
`

export const NetworkButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 8px;
  background-color: transparent;
  width: 100%;
  border-radius: 8px;
  &:hover {
    background-color: ${leo.color.divider.subtle};
  }
`

export const ActiveIndicator = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  border-radius: 4px;
  padding: 2px 6px;
  background-color: ${leo.color.primary[20]};
  color: ${leo.color.primary[50]};
  font-family: 'Poppins';
  font-weight: 600;
  font-size: 10px;
  line-height: 15px;
  text-transform: uppercase;
  letter-spacing: 0.4px;
`

export const BackButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 36px;
  height: 36px;
  border-radius: 100%;
  outline: none;
  border: 1px solid ${leo.color.divider.interactive};
  background: none;
  padding: 0px;
  margin: 0px 16px 0px 0px;
  background-color: transparent;
`

export const BackIcon = styled(Icon).attrs({
  name: 'carat-left'
})`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.interactive};
`

export const NetworkIconWrapper = styled.div<{
  showConnectionStatus: boolean
}>`
  position: absolute;
  right: ${(p) => (p.showConnectionStatus ? 12 : 2)}px;
  bottom: 4px;
  border: 2px solid ${leo.color.container.background};
  border-radius: 100%;
`

export const FavIcon = styled.img<{
  size: string
  marginRight?: string
}>`
  width: ${(p) => p.size};
  height: ${(p) => p.size};
  border-radius: 5px;
  margin-right: ${(p) => p.marginRight ?? '0px'};
`
