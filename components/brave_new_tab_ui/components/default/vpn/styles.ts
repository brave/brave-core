/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { color, font, gradient, spacing } from '@brave/leo/tokens/css/variables'
import guardianLogoUrl from '../vpn/assets/guardian-logo.svg'
import vpnShieldsConnectedUrl from '../vpn/assets/vpn-shields-connected.svg'
import vpnShieldsDisconnectedUrl from '../vpn/assets/vpn-shields-disconnected.svg'
import vpnShieldsConnectingUrl from '../vpn/assets/vpn-shields-connecting.svg'
import { ConnectionState } from '../../../api/braveVpn'

export const HeaderIcon = styled(Icon)`
  --leo-icon-size: 24px;
  --leo-icon-color: ${gradient.iconsActive};
`

export const StyledTitle = styled.div`
  font: ${font.heading.h4};
  color: ${color.white};
  display: flex;
  align-items: center;
  gap: 8px;
`

export const WidgetWrapper = styled.div.attrs({ 'data-theme': 'dark' })`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  align-self: stretch;
  color: ${color.white};
  gap: ${spacing['2Xl']};
`

export const WidgetContent = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16px;
  align-self: stretch;
`

export const PoweredBy = styled.div`
  display: flex;
  align-items: center;
  opacity: 0.5;
  gap: 4px;

  span {
    color: ${color.white};
    font: ${font.xSmall.regular};
    text-align: center;
  }
`

export const GuardianLogo = styled.span`
  width: 56px;
  height: 12px;
  background-image: url(${guardianLogoUrl});
`

export const SellingPoints = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: ${spacing.s};
  padding-left: ${spacing.xs};
  align-self: stretch;
`

export const SellingPoint = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
`

export const SellingPointIcon = styled(Icon)`
  align-self: start;
  margin: 1px;
  --leo-icon-size: ${spacing.l};
  --leo-icon-color: ${color.icon.disabled};
`

export const SellingPointLabel = styled.span`
  color: ${color.text.primary};
  font: ${font.xSmall.regular};
`

export const ActionArea = styled.div`
  display: flex;
  padding: 0px;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  gap: ${spacing.s};
  align-self: stretch;
`

export const ActionButton = styled(Button)`
  align-self: stretch;
`

export const ActionLabel = styled.div`
  color: #fff;
  opacity: 0.5;
  font: ${font.xSmall.regular};
`

export const VPNShileldsIcon = styled.div<{ connectionState: ConnectionState }>`
  width: 62px;
  height: 62px;
  background-image: url(${(p) =>
    p.connectionState === ConnectionState.CONNECTED
      ? vpnShieldsConnectedUrl
      : p.connectionState === ConnectionState.CONNECTING
      ? vpnShieldsConnectingUrl
      : vpnShieldsDisconnectedUrl});
`

export const ActionBox = styled.div`
  display: flex;
  align-items: center;
  gap: 23px;
  align-self: stretch;
`

export const ConnectionInfoBox = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: ${spacing.xs};
  flex: 1 0 0;
`

export const ConnectionStateLabel = styled.span<{ connected: boolean }>`
  align-self: stretch;
  color: ${(p) => (p.connected ? color.green[60] : color.text.primary)};
  font: ${font.small.regular};
`

export const RegionAction = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};
`

export const RegionCountryLabel = styled.span`
  color: ${color.text.primary};
  font: ${font.large.semibold};
`

export const RegionChangeLink = styled.a`
  color: ${color.text.primary};
  font: ${font.xSmall.link};
  text-decoration-line: underline;
  cursor: pointer;
`

export const RegionCityLabel = styled.span`
  color: ${color.text.tertiary};
  font: ${font.xSmall.regular};
`
