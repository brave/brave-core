/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useDispatch } from 'react-redux'

import Toggle from '@brave/leo/react/toggle'
import * as S from './styles'
import VPNShieldsConnecting from './vpn-shields-connecting'
import * as Actions from '../../../actions/brave_vpn_actions'
import { ConnectionState, Region } from '../../../api/braveVpn'

// Delete this.
const locales = {
  firewallVpn: 'Brave VPN',
  connected: 'Connected',
  connecting: 'Connecting',
  disconnected: 'Disconnected',
  disconnecting: 'Disconnecting',
  change: 'Change',
  optimal: 'Optimal',

  promo: {
    heading: 'Extra privacy & security online',
    poweredBy: 'Powered by',
    cta: 'Start free trial',
    freeTrial: '7-day free trial'
  }
}

export const VPNWidgetTitle = () => {
  return (
    <>
      <S.HeaderIcon name='product-vpn' />
      {locales.firewallVpn}
    </>
  )
}

export const VPNWidgetHeader = () => {
  return (
    <S.StyledTitle>
      <VPNWidgetTitle />
    </S.StyledTitle>
  )
}

export const VPNPromoWidget = () => {
  const dispatch = useDispatch()

  const featureList = React.useMemo(
    () => [
      'Extra privacy & security online',
      'Hide your IP & change your location',
      'Protect every app on your device'
    ],
    []
  )

  return (
    <S.WidgetWrapper>
      <VPNWidgetHeader />
      <S.WidgetContent>
        <S.PoweredBy>
          <span>{locales.promo.poweredBy}</span>
          <S.GuardianLogo />
        </S.PoweredBy>
        <S.SellingPoints>
          {featureList.map((entry, i) => (
            <S.SellingPoint key={i}>
              <S.SellingPointIcon name='shield-done' />
              <S.SellingPointLabel>{entry}</S.SellingPointLabel>
            </S.SellingPoint>
          ))}
        </S.SellingPoints>
        <S.ActionArea>
          <S.ActionButton
            onClick={() => dispatch(Actions.openVPNAccountPage())}
          >
            {locales.promo.cta}
          </S.ActionButton>
          <S.ActionLabel>{locales.promo.freeTrial}</S.ActionLabel>
        </S.ActionArea>
      </S.WidgetContent>
    </S.WidgetWrapper>
  )
}

function GetConnectionStateLabel(connectionState: ConnectionState) {
  switch (connectionState) {
    case ConnectionState.CONNECTED:
      return locales.connected
    case ConnectionState.CONNECTING:
      return locales.connecting
    case ConnectionState.DISCONNECTING:
      return locales.disconnecting
    default:
      break
  }

  return locales.disconnected
}

interface MainWidgetProps {
  connectionState: ConnectionState
  selectedRegion: Region
}

export const VPNMainWidget = (props: MainWidgetProps) => {
  const dispatch = useDispatch()
  const connected = props.connectionState === ConnectionState.CONNECTED

  return (
    <S.WidgetWrapper>
      <VPNWidgetHeader />
      <S.WidgetContent>
        {props.connectionState === ConnectionState.CONNECTING ? (
          <VPNShieldsConnecting />
        ) : (
          <S.VPNShileldsIcon connectionState={props.connectionState} />
        )}
        <S.ActionBox>
          <S.ConnectionInfoBox>
            <S.ConnectionStateLabel connected={connected}>
              {GetConnectionStateLabel(props.connectionState)}
            </S.ConnectionStateLabel>
            <S.RegionAction>
              <S.RegionCountryLabel>
                {props.selectedRegion.country}
              </S.RegionCountryLabel>
              <S.RegionChangeLink
                onClick={() => dispatch(Actions.launchVPNPanel())}
              >
                {locales.change}
              </S.RegionChangeLink>
            </S.RegionAction>
            <S.RegionCityLabel>
              {props.selectedRegion.namePretty === props.selectedRegion.country
                ? locales.optimal
                : props.selectedRegion.namePretty}
            </S.RegionCityLabel>
          </S.ConnectionInfoBox>
          <Toggle
            checked={
              props.connectionState === ConnectionState.CONNECTED ||
              props.connectionState === ConnectionState.CONNECTING
            }
            onChange={() => dispatch(Actions.toggleConnection())}
          />
        </S.ActionBox>
      </S.WidgetContent>
    </S.WidgetWrapper>
  )
}
