// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import * as Styles from './style'
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
      ? getLocale(S.BRAVE_VPN_SERVER_SELECTION_SINGLE_CITY)
      : getLocale(S.BRAVE_VPN_SERVER_SELECTION_MULTIPLE_CITIES).replace(
          '$1',
          `${numCity}`
        )

  return `${city} - ${getCityInfo(numServer)}`
}

function getCityInfo(numServer: number) {
  return numServer === 1
    ? getLocale(S.BRAVE_VPN_SERVER_SELECTION_SINGLE_SERVER)
    : getLocale(S.BRAVE_VPN_SERVER_SELECTION_MULTIPLE_SERVERS).replace(
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
    <Styles.RegionConnect
      slot='actions'
      kind='filled'
      size='tiny'
      right={props.right}
      onClick={(event) => {
        event.stopPropagation()
        props.connect()
      }}
    >
      {getLocale(S.BRAVE_VPN_CONNECT)}
    </Styles.RegionConnect>
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
    <Styles.RegionCity selected={props.selected}>
      <Styles.RegionCityInfo>
        <Styles.RegionCityLabel selected={props.selected}>
          {props.cityLabel}
        </Styles.RegionCityLabel>
        <Styles.CityServerInfo selected={props.selected}>
          {props.serverInfo}
        </Styles.CityServerInfo>
      </Styles.RegionCityInfo>
      {props.selected && (
        <Styles.StyledCheckBox name='check-circle-filled'></Styles.StyledCheckBox>
      )}
      {showButton && props.connectionButton}
    </Styles.RegionCity>
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
    <Styles.RegionContainer
      selected={props.selected}
      fillBackground={!props.open}
      ref={ref}
    >
      <Styles.RegionCountry
        onClick={() => {
          // Pass '' to toggle currently opened country.
          props.onClick(props.open ? '' : props.region.name)
        }}
      >
        <Flag countryCode={props.region.countryIsoCode} />
        <Styles.CountryInfo>
          <RegionLabelBox>
            <Styles.RegionCountryLabel selected={!props.open && props.selected}>
              {props.region.namePretty}
            </Styles.RegionCountryLabel>
            {smartProxyRoutingEnabled && props.region.smartRoutingProxyState === 'all' && (
              <Tooltip mode='mini'>
                <SmartProxyIcon name='smart-proxy-routing' />
                <div slot='content'>
                  {getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING_ICON_TOOLTIP)}
                </div>
              </Tooltip>)
            }
          </RegionLabelBox>
          <Styles.CountryServerInfo selected={!props.open && props.selected}>
            {getCountryInfo(
              props.region.cities.length,
              props.region.serverCount
            )}
          </Styles.CountryServerInfo>
        </Styles.CountryInfo>
        {props.selected && (
          <Styles.StyledCheckBox name='check-circle-filled'></Styles.StyledCheckBox>
        )}
        <Styles.StyledIcon
          name={props.open ? 'carat-up' : 'carat-down'}
        ></Styles.StyledIcon>
        {!props.open && !props.selected && (
          <ConnectButton
            right='44px'
            connect={() => handleConnect(props.region)}
          />
        )}
      </Styles.RegionCountry>
      {props.open && (
        <>
          <RegionCity
            cityLabel={getLocale(S.BRAVE_VPN_SERVER_SELECTION_OPTIMAL_LABEL)}
            serverInfo={getLocale(S.BRAVE_VPN_SERVER_SELECTION_OPTIMAL_DESC)}
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
    </Styles.RegionContainer>
  )
}

function RegionAutomatic() {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)
  const handleConnect = () => {
    dispatch(Actions.connectToNewRegionAutomatically())
  }

  return (
    <Styles.RegionContainer
      selected={currentRegion.isAutomatic}
      fillBackground={true}
    >
      <Styles.RegionCountry>
        <Flag countryCode='WORLDWIDE' />
        <Styles.CountryInfo>
          <Styles.RegionCountryLabel selected={currentRegion.isAutomatic}>
            {getLocale(S.BRAVE_VPN_SERVER_SELECTION_AUTOMATIC_LABEL)}
          </Styles.RegionCountryLabel>
          <Styles.CountryServerInfo selected={currentRegion.isAutomatic}>
            {getLocale(S.BRAVE_VPN_SERVER_SELECTION_OPTIMAL_DESC)}
          </Styles.CountryServerInfo>
        </Styles.CountryInfo>
        {currentRegion.isAutomatic ? (
          <Styles.StyledCheckBox name='check-circle-filled'></Styles.StyledCheckBox>
        ) : (
          <ConnectButton
            right='16px'
            connect={handleConnect}
          />
        )}
      </Styles.RegionCountry>
    </Styles.RegionContainer>
  )
}

export function PanelHeader(props: {
  title: string
  buttonAriaLabel: string
  onClick: () => void
}) {
  return (
    <Styles.PanelHeader>
      <Styles.StyledButton
        type='button'
        onClick={props.onClick}
        aria-label={props.buttonAriaLabel}
        title={props.buttonAriaLabel}
      >
        <Styles.StyledIcon name='arrow-left'></Styles.StyledIcon>
      </Styles.StyledButton>
      <Styles.HeaderLabel>{props.title}</Styles.HeaderLabel>
    </Styles.PanelHeader>
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
    <Styles.Box>
      <Styles.PanelContent>
        <PanelHeader
          title={getLocale(S.BRAVE_VPN_SELECT_YOUR_SERVER)}
          buttonAriaLabel={getLocale(S.BRAVE_VPN_SETTINGS_PANEL_BACK_BUTTON_ARIA_LABEL)}
          onClick={handleGoBackClick}
        />
        <Styles.Divider />
        <Styles.RegionList>
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
        </Styles.RegionList>
      </Styles.PanelContent>
    </Styles.Box>
  )
}

export default SelectRegion
