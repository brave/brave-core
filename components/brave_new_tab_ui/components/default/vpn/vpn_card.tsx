/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useDispatch } from 'react-redux'
import styled from 'styled-components'

import Toggle from '@brave/leo/react/toggle'
import { getLocale } from '$web-common/locale'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { color, font, gradient, spacing } from '@brave/leo/tokens/css/variables'
import VPNShieldsConnecting from './vpn-shields-connecting'
import * as Actions from '../../../actions/brave_vpn_actions'
import { ConnectionState, Region } from '../../../api/braveVpn'
import guardianLogoUrl from '../vpn/assets/guardian-logo.svg'
import vpnShieldsConnectedUrl from '../vpn/assets/vpn-shields-connected.svg'
import vpnShieldsDisconnectedUrl from '../vpn/assets/vpn-shields-disconnected.svg'

const HeaderIcon = styled(Icon)`
  --leo-icon-size: 24px;
  --leo-icon-color: ${gradient.iconsActive};
`

const StyledTitle = styled.div`
  font: ${font.heading.h4};
  color: ${color.white};
  display: flex;
  align-items: center;
  gap: 8px;
`

const WidgetWrapper = styled.div.attrs({ 'data-theme': 'dark' })`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  align-self: stretch;
  color: ${color.white};
  gap: ${spacing['2Xl']};
`

const WidgetContent = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16px;
  align-self: stretch;
`

const PoweredBy = styled.div`
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

const GuardianLogo = styled.span`
  width: 56px;
  height: 12px;
  background-image: url(${guardianLogoUrl});
`

const SellingPoints = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: ${spacing.s};
  padding-left: ${spacing.xs};
  align-self: stretch;
`

const SellingPoint = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
`

const SellingPointIcon = styled(Icon)`
  align-self: start;
  margin: 1px;
  --leo-icon-size: ${spacing.l};
  --leo-icon-color: ${color.icon.disabled};
`

const SellingPointLabel = styled.span`
  color: ${color.text.primary};
  font: ${font.xSmall.regular};
`

const ActionArea = styled.div`
  display: flex;
  padding: 0px;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  gap: ${spacing.s};
  align-self: stretch;
`

const ActionButton = styled(Button)`
  align-self: stretch;
`

const ActionLabel = styled.div`
  color: #fff;
  opacity: 0.5;
  font: ${font.xSmall.regular};
`

const VPNShileldsIcon = styled.div<{ connectionState: ConnectionState }>`
  width: 62px;
  height: 62px;
  margin-bottom: 3px;
  background-image: url(${(p) =>
    p.connectionState === ConnectionState.CONNECTED
      ? vpnShieldsConnectedUrl
      : vpnShieldsDisconnectedUrl});
`

const ActionBox = styled.div`
  display: flex;
  align-items: center;
  gap: 23px;
  align-self: stretch;
`

const ConnectionInfoBox = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: ${spacing.xs};
  flex: 1 0 0;
`

const ConnectionStateLabel = styled.span<{ connected: boolean }>`
  align-self: stretch;
  color: ${(p) => (p.connected ? color.green[60] : color.text.primary)};
  font: ${font.small.regular};
`

const RegionAction = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};
`

const RegionCountryLabel = styled.span`
  color: ${color.text.primary};
  font: ${font.large.semibold};
  height: 24px;
`

const RegionChangeLink = styled.a`
  color: ${color.text.primary};
  font: ${font.xSmall.link};
  text-decoration-line: underline;
  cursor: pointer;
`

const RegionCityLabel = styled.span`
  color: ${color.text.tertiary};
  font: ${font.xSmall.regular};
`

export const VPNWidgetTitle = () => {
  return (
    <>
      <HeaderIcon name='product-vpn' />
      {getLocale('braveVpnWidgetTitle')}
    </>
  )
}

const VPNWidgetHeader = () => {
  return (
    <StyledTitle>
      <VPNWidgetTitle />
    </StyledTitle>
  )
}

const FeatureList = () => (
  <SellingPoints>
    {[
      getLocale('braveVpnFeature1'),
      getLocale('braveVpnFeature2'),
      getLocale('braveVpnWidgetFeature3')
    ].map((entry) => (
      <SellingPoint key={entry}>
        <SellingPointIcon name='shield-done' />
        <SellingPointLabel>{entry}</SellingPointLabel>
      </SellingPoint>
    ))}
  </SellingPoints>
)

export const VPNPromoWidget = () => {
  const dispatch = useDispatch()

  return (
    <WidgetWrapper>
      <VPNWidgetHeader />
      <WidgetContent>
        <PoweredBy>
          <span>{getLocale('braveVpnPoweredBy')}</span>
          <GuardianLogo />
        </PoweredBy>
        <FeatureList />
        <ActionArea>
          <ActionButton onClick={() => dispatch(Actions.openVPNAccountPage())}>
            {getLocale('braveVpnCTA')}
          </ActionButton>
          <ActionLabel>{getLocale('braveVpnFreeTrial')}</ActionLabel>
        </ActionArea>
      </WidgetContent>
    </WidgetWrapper>
  )
}

function GetConnectionStateLabel(connectionState: ConnectionState) {
  switch (connectionState) {
    case ConnectionState.CONNECTED:
      return getLocale('braveVpnConnected')
    case ConnectionState.CONNECTING:
      return getLocale('braveVpnConnecting')
    case ConnectionState.DISCONNECTING:
      return getLocale('braveVpnDisconnecting')
    default:
      break
  }

  return getLocale('braveVpnDisconnected')
}

interface MainWidgetProps {
  connectionState: ConnectionState
  selectedRegion: Region
}

export const VPNMainWidget = (props: MainWidgetProps) => {
  const dispatch = useDispatch()
  const connected = props.connectionState === ConnectionState.CONNECTED

  return (
    <WidgetWrapper>
      <VPNWidgetHeader />
      <WidgetContent>
        {props.connectionState === ConnectionState.CONNECTING ? (
          <VPNShieldsConnecting />
        ) : (
          <VPNShileldsIcon connectionState={props.connectionState} />
        )}
        <ActionBox>
          <ConnectionInfoBox>
            <ConnectionStateLabel connected={connected}>
              {GetConnectionStateLabel(props.connectionState)}
            </ConnectionStateLabel>
            <RegionAction>
              <RegionCountryLabel>
                {props.selectedRegion.country}
              </RegionCountryLabel>
              <RegionChangeLink
                onClick={() => dispatch(Actions.launchVPNPanel())}
              >
                {getLocale('braveVpnChangeRegion')}
              </RegionChangeLink>
            </RegionAction>
            <RegionCityLabel>
              {props.selectedRegion.namePretty === props.selectedRegion.country
                ? getLocale('braveVpnOptimal')
                : props.selectedRegion.namePretty}
            </RegionCityLabel>
          </ConnectionInfoBox>
          <Toggle
            checked={
              props.connectionState === ConnectionState.CONNECTED ||
              props.connectionState === ConnectionState.CONNECTING
            }
            onChange={() => dispatch(Actions.toggleConnection())}
          />
        </ActionBox>
      </WidgetContent>
    </WidgetWrapper>
  )
}
