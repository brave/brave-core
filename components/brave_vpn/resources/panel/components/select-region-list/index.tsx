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

interface RegionContentProps {
  region: Region
  selected: boolean
}

function RegionContent(props: RegionContentProps) {
  const dispatch = useDispatch()
  const handleConnect = () => {
    dispatch(Actions.connectToNewRegion(props.region))
  }

  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (props.selected) ref.current?.scrollIntoView()
  }, [])

  return (
    <S.RegionContainer
      selected={props.selected}
      ref={ref}
    >
      <S.RegionCountry selected={props.selected}>
        <Flag countryCode={props.region.countryIsoCode} />
        <S.RegionCountryLabel>
          {props.region.namePretty}
        </S.RegionCountryLabel>
        {props.selected && (
          <S.StyledCheckBox name='check-circle-filled'></S.StyledCheckBox>
        )}
        <S.RegionConnect
          slot='actions'
          kind='filled'
          size='tiny'
          onClick={handleConnect}
          id='connect'
        >
          {getLocale('braveVpnConnect')}
        </S.RegionConnect>
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

  const handleGoBackClick = () => {
    dispatch(Actions.toggleRegionSelector(false))
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
          {regions.map((region: Region) => (
            <RegionContent
              key={region.name}
              region={region}
              selected={currentRegion.name === region.name}
            />
          ))}
        </S.RegionList>
      </S.PanelContent>
    </S.Box>
  )
}

export default SelectRegion
