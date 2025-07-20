// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import * as S from './style'
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
    <S.RegionConnect
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
    </S.RegionConnect>
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
    <S.RegionCity selected={props.selected}>
      <S.RegionCityInfo>
        <S.RegionCityLabel selected={props.selected}>
          {props.cityLabel}
        </S.RegionCityLabel>
        <S.CityServerInfo selected={props.selected}>
          {props.serverInfo}
        </S.CityServerInfo>
      </S.RegionCityInfo>
      {props.selected && (
        <S.StyledCheckBox name='check-circle-filled'></S.StyledCheckBox>
      )}
      {showButton && props.connectionButton}
    </S.RegionCity>
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
    <S.RegionContainer
      selected={props.selected}
      fillBackground={!props.open}
      ref={ref}
    >
      <S.RegionCountry
        onClick={() => {
          // Pass '' to toggle currently opened country.
          props.onClick(props.open ? '' : props.region.name)
        }}
      >
        <Flag countryCode={props.region.countryIsoCode} />
        <S.CountryInfo>
          <RegionLabelBox>
            <S.RegionCountryLabel selected={!props.open && props.selected}>
              {props.region.namePretty}
            </S.RegionCountryLabel>
            {smartProxyRoutingEnabled && props.region.smartRoutingProxyState === 'all' && (
              <Tooltip mode='mini'>
                <SmartProxyIcon name='smart-proxy-routing' />
                <div slot='content'>
                  {getLocale('braveVpnSmartProxyRoutingIconTooltip')}
                </div>
              </Tooltip>)
            }
          </RegionLabelBox>
          <S.CountryServerInfo selected={!props.open && props.selected}>
            {getCountryInfo(
              props.region.cities.length,
              props.region.serverCount
            )}
          </S.CountryServerInfo>
        </S.CountryInfo>
        {props.selected && (
          <S.StyledCheckBox name='check-circle-filled'></S.StyledCheckBox>
        )}
        <S.StyledIcon
          name={props.open ? 'carat-up' : 'carat-down'}
        ></S.StyledIcon>
        {!props.open && !props.selected && (
          <ConnectButton
            right='44px'
            connect={() => handleConnect(props.region)}
          />
        )}
      </S.RegionCountry>
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
    </S.RegionContainer>
  )
}

function RegionAutomatic() {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)
  const handleConnect = () => {
    dispatch(Actions.connectToNewRegionAutomatically())
  }

  return (
    <S.RegionContainer
      selected={currentRegion.isAutomatic}
      fillBackground={true}
    >
      <S.RegionCountry>
        <Flag countryCode='WORLDWIDE' />
        <S.CountryInfo>
          <S.RegionCountryLabel selected={currentRegion.isAutomatic}>
            {getLocale('braveVpnServerSelectionAutomaticLabel')}
          </S.RegionCountryLabel>
          <S.CountryServerInfo selected={currentRegion.isAutomatic}>
            {getLocale('braveVpnServerSelectionOptimalDesc')}
          </S.CountryServerInfo>
        </S.CountryInfo>
        {currentRegion.isAutomatic ? (
          <S.StyledCheckBox name='check-circle-filled'></S.StyledCheckBox>
        ) : (
          <ConnectButton
            right='16px'
            connect={handleConnect}
          />
        )}
      </S.RegionCountry>
    </S.RegionContainer>
  )
}

export function PanelHeader(props: {
  title: string
  buttonAriaLabel: string
  onClick: () => void
}) {
  return (
    <S.PanelHeader>
      <S.StyledButton
        type='button'
        onClick={props.onClick}
        aria-label={props.buttonAriaLabel}
        title={props.buttonAriaLabel}
      >
        <S.StyledIcon name='arrow-left'></S.StyledIcon>
      </S.StyledButton>
      <S.HeaderLabel>{props.title}</S.HeaderLabel>
    </S.PanelHeader>
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
    <S.Box>
      <S.PanelContent>
        <PanelHeader
          title={getLocale('braveVpnSelectYourServer')}
          buttonAriaLabel={getLocale('braveVpnSelectPanelBackButtonAriaLabel')}
          onClick={handleGoBackClick}
        />
        <S.Divider />
        <S.RegionList>
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
        </S.RegionList>
      </S.PanelContent>
    </S.Box>
  )
}

export default SelectRegion
