// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import Flag from '../flag'
import { Region } from '../../api/panel_browser_api'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import { getLocale } from '$web-common/locale'

import 'emptykit.css'

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
      {props.connectionButton}
    </S.RegionCity>
  )
}

interface RegionContentProps {
  region: Region
  selected: boolean
  open: boolean
  clickHandler: (countryName: string) => void
}

function RegionContent(props: RegionContentProps) {
  const dispatch = useDispatch()
  const currentRegion = useSelector((state) => state.currentRegion)
  const handleConnect = (region: Region) => {
    dispatch(Actions.connectToNewRegion(region))
  }

  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (props.selected) ref.current?.scrollIntoView()
  }, [])

  const countryInfo = {
    $1: props.region.cities.length,
    $2: props.region.serverCount
  }

  return (
    <S.RegionContainer
      selected={props.selected}
      fillBackground={!props.open}
      ref={ref}
    >
      <S.RegionCountry
        onClick={() => {
          // Pass '' to toggle currently opened country.
          props.clickHandler(props.open ? '' : props.region.name)
        }}
      >
        <Flag countryCode={props.region.countryIsoCode} />
        <S.CountryInfo>
          <S.RegionCountryLabel selected={!props.open && props.selected}>
            {props.region.namePretty}
          </S.RegionCountryLabel>
          <S.CountryServerInfo selected={!props.open && props.selected}>
            {getLocale('braveVpnServerSelectionCountryInfo').replace(
              /\$\d+/g,
              (match) => countryInfo[match]
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
            selected={props.selected && currentRegion.name === props.region.name}
            connectionButton={ConnectButton({
              right: '16px',
              connect: () => handleConnect(props.region)
            })}
          />
          {props.region.cities.map((city: Region) => (
            <RegionCity
              key={city.name}
              cityLabel={city.namePretty}
              serverInfo={getLocale('braveVpnServerSelectionCityInfo').replace(
                '$1',
                `${city.serverCount}`
              )}
              selected={props.selected && currentRegion.name === city.name}
              connectionButton={ConnectButton({
                right: '16px',
                connect: () => handleConnect(city)
              })}
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
        <Flag countryCode={'WORLDWIDE'} />
        <S.CountryInfo>
          <S.RegionCountryLabel selected={currentRegion.isAutomatic}>
            {getLocale('braveVpnServerSelectionAutomaticLabel')}
          </S.RegionCountryLabel>
          <S.CountryServerInfo selected={currentRegion.isAutomatic}>
            {getLocale('braveVpnServerSelectionOptimalDesc')}
          </S.CountryServerInfo>
        </S.CountryInfo>
        {currentRegion.isAutomatic && (
          <S.StyledCheckBox name='check-circle-filled'></S.StyledCheckBox>
        )}
        {!currentRegion.isAutomatic && (
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
              clickHandler={(countryName) => setOpenedCountry(countryName)}
            />
          ))}
        </S.RegionList>
      </S.PanelContent>
    </S.Box>
  )
}

export default SelectRegion
