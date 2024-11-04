/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useDispatch } from 'react-redux'

import Toggle from '@brave/leo/react/toggle'
import { getLocale } from '$web-common/locale'
import * as S from './styles'
import VPNShieldsConnecting from './vpn-shields-connecting'
import * as Actions from '../../../actions/brave_vpn_actions'
import { ConnectionState, Region } from '../../../api/braveVpn'

export const VPNWidgetTitle = () => {
  return (
    <>
      <S.HeaderIcon name='product-vpn' />
      {getLocale('braveVpnWidgetTitle')}
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
      getLocale('braveVpnFeature1'),
      getLocale('braveVpnFeature2'),
      getLocale('braveVpnWidgetFeature3')
    ],
    []
  )

  return (
    <S.WidgetWrapper>
      <VPNWidgetHeader />
      <S.WidgetContent>
        <S.PoweredBy>
          <span>{getLocale('braveVpnPoweredBy')}</span>
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
            {getLocale('braveVpnCTA')}
          </S.ActionButton>
          <S.ActionLabel>{getLocale('braveVpnFreeTrial')}</S.ActionLabel>
        </S.ActionArea>
      </S.WidgetContent>
    </S.WidgetWrapper>
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
                {getLocale('braveVpnChangeRegion')}
              </S.RegionChangeLink>
            </S.RegionAction>
            <S.RegionCityLabel>
              {props.selectedRegion.namePretty === props.selectedRegion.country
                ? getLocale('braveVpnOptimal')
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
