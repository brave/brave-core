// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import * as Style from './style'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import Flag from '../flag'
import { color } from '@brave/leo/tokens/css/variables'
import { ConnectionState, Region } from '../../api/panel_browser_api'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import { getLocale } from '$web-common/locale'

import 'emptykit.css'

const RegionLabelBox = styled.span`
  display: flex;
  align-items: center;
  gap: 10px;
`

const SmartProxyIcon = styled(Icon)`
  --leo-icon-size: 18px;
  --leo-icon-color: ${color.icon.default};
  background: ${color.container.highlight};
`

function getCountryInfo(numCity: number, numServer: number) {
  const city =
    numCity === 1
      ? getLocale('braveVpnServerSelectionSingleCity')
      : getLocale('braveVpnServerSelectionMultipleCities').replace(
          '$1',
          `${numCity}`
        )

  return `${city} - ${getCityInfo(numServer)}`
}

function getCityInfo(numServer: number) {
  return numServer === 1
    ? getLocale('braveVpnServerSelectionSingleServer')
    : getLocale('braveVpnServerSelectionMultipleServers').replace(
        '$1',
        `${numServer}`
      )
}

interface ConnectButtonProps {
  right: string
  connect: () => void
}

function ConnectButton(props: ConnectButtonProps) {
  return (
    <Style.RegionConnect
      slot='actions'
      kind='filled'
      size='tiny'
      right={props.right}
      onClick={(event) => {
        event.stopPropagation()
        props.connect()
      }}
    >
      {getLocale('braveVpnConnect')}
    </Style.RegionConnect>
  )
}

interface RegionCityProps {
  cityLabel: string
  serverInfo: string
  selected: boolean
  connectionButton: React.ReactElement
}

function RegionCity(props: RegionCityProps) {
  const connectionStatus = useSelector((state) => state.connectionStatus)
  const showButton =
    !props.selected || connectionStatus !== ConnectionState.CONNECTED
  return (
    <Style.RegionCity selected={props.selected}>
      <Style.RegionCityInfo>
        <Style.RegionCityLabel selected={props.selected}>
          {props.cityLabel}
        </Style.RegionCityLabel>
        <Style.CityServerInfo selected={props.selected}>
          {props.serverInfo}
        </Style.CityServerInfo>
      </Style.RegionCityInfo>
      {props.selected && (
        <Style.StyledCheckBox name='check-circle-filled'></Style.StyledCheckBox>
      )}
      {showButton && props.connectionButton}
    </Style.RegionCity>
  )
}

interface RegionContentProps {
  region: Region
  selected: boolean
  open: boolean
  onClick: (countryName: string) => void
}

function RegionContent(props: RegionContentProps) {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)
  const smartProxyRoutingEnabled = useSelector((state) => state.smartProxyRoutingEnabled)
  const handleConnect = (region: Region) => {
    dispatch(Actions.connectToNewRegion(region))
  }

  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (props.selected) ref.current?.scrollIntoView()
  }, [])

  return (
    <Style.RegionContainer
      selected={props.selected}
      fillBackground={!props.open}
      ref={ref}
    >
      <Style.RegionCountry
        onClick={() => {
          // Pass '' to toggle currently opened country.
          props.onClick(props.open ? '' : props.region.name)
        }}
      >
        <Flag countryCode={props.region.countryIsoCode} />
        <Style.CountryInfo>
          <RegionLabelBox>
            <Style.RegionCountryLabel selected={!props.open && props.selected}>
              {props.region.namePretty}
            </Style.RegionCountryLabel>
            {smartProxyRoutingEnabled && props.region.smartRoutingProxyState === 'all' && (
              <Tooltip mode='mini'>
                <SmartProxyIcon name='smart-proxy-routing' />
                <div slot='content'>
                  {getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING_ICON_TOOLTIP)}
                </div>
              </Tooltip>)
            }
          </RegionLabelBox>
          <Style.CountryServerInfo selected={!props.open && props.selected}>
            {getCountryInfo(
              props.region.cities.length,
              props.region.serverCount
            )}
          </Style.CountryServerInfo>
        </Style.CountryInfo>
        {props.selected && (
          <Style.StyledCheckBox name='check-circle-filled'></Style.StyledCheckBox>
        )}
        <Style.StyledIcon
          name={props.open ? 'carat-up' : 'carat-down'}
        ></Style.StyledIcon>
        {!props.open && !props.selected && (
          <ConnectButton
            right='44px'
            connect={() => handleConnect(props.region)}
          />
        )}
      </Style.RegionCountry>
      {props.open && (
        <>
          <RegionCity
            cityLabel={getLocale('braveVpnServerSelectionOptimalLabel')}
            serverInfo={getLocale('braveVpnServerSelectionOptimalDesc')}
            selected={
              props.selected && currentRegion.name === props.region.name
            }
            connectionButton={
              <ConnectButton
                right='16px'
                connect={() => handleConnect(props.region)}
              />
            }
          />
          {props.region.cities.map((city: Region) => (
            <RegionCity
              key={city.name}
              cityLabel={city.namePretty}
              serverInfo={getCityInfo(city.serverCount)}
              selected={props.selected && currentRegion.name === city.name}
              connectionButton={
                <ConnectButton
                  right='16px'
                  connect={() => handleConnect(city)}
                />
              }
            />
          ))}
        </>
      )}
    </Style.RegionContainer>
  )
}

function RegionAutomatic() {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)
  const handleConnect = () => {
    dispatch(Actions.connectToNewRegionAutomatically())
  }

  return (
    <Style.RegionContainer
      selected={currentRegion.isAutomatic}
      fillBackground={true}
    >
      <Style.RegionCountry>
        <Flag countryCode='WORLDWIDE' />
        <Style.CountryInfo>
          <Style.RegionCountryLabel selected={currentRegion.isAutomatic}>
            {getLocale('braveVpnServerSelectionAutomaticLabel')}
          </Style.RegionCountryLabel>
          <Style.CountryServerInfo selected={currentRegion.isAutomatic}>
            {getLocale('braveVpnServerSelectionOptimalDesc')}
          </Style.CountryServerInfo>
        </Style.CountryInfo>
        {currentRegion.isAutomatic ? (
          <Style.StyledCheckBox name='check-circle-filled'></Style.StyledCheckBox>
        ) : (
          <ConnectButton
            right='16px'
            connect={handleConnect}
          />
        )}
      </Style.RegionCountry>
    </Style.RegionContainer>
  )
}

export function PanelHeader(props: {
  title: string
  buttonAriaLabel: string
  onClick: () => void
}) {
  return (
    <Style.PanelHeader>
      <Style.StyledButton
        type='button'
        onClick={props.onClick}
        aria-label={props.buttonAriaLabel}
        title={props.buttonAriaLabel}
      >
        <Style.StyledIcon name='arrow-left'></Style.StyledIcon>
      </Style.StyledButton>
      <Style.HeaderLabel>{props.title}</Style.HeaderLabel>
    </Style.PanelHeader>
  )
}

function SelectRegion() {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)
  const regions = useSelector((state) => state.regions)
  const [openedCountry, setOpenedCountry] = React.useState('')

  const handleGoBackClick = () => {
    dispatch(Actions.toggleRegionSelector(false))
  }

  const hasCurrentRegion = (region: Region) => {
    if (currentRegion.name === region.name) {
      return true
    }

    return region.cities.some((city) => city.name === currentRegion.name)
  }

  if (!currentRegion) {
    console.error('Selected region is not defined')
  }

  return (
    <Style.Box>
      <Style.PanelContent>
        <PanelHeader
          title={getLocale('braveVpnSelectYourServer')}
          buttonAriaLabel={getLocale('braveVpnSelectPanelBackButtonAriaLabel')}
          onClick={handleGoBackClick}
        />
        <Style.Divider />
        <Style.RegionList>
          <RegionAutomatic />
          {regions.map((region: Region) => (
            <RegionContent
              key={region.name}
              region={region}
              selected={!currentRegion.isAutomatic && hasCurrentRegion(region)}
              open={region.name === openedCountry}
              onClick={(countryName) => setOpenedCountry(countryName)}
            />
          ))}
        </Style.RegionList>
      </Style.PanelContent>
    </Style.Box>
  )
}

export default SelectRegion
