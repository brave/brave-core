// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import * as S from './style'
import { color, font } from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'
import { formatMessage } from '../../../../../brave_rewards/resources/shared/lib/locale_context'
import SelectRegionList from '../select-region-list'
import PanelBox from '../panel-box'
import Toggle from '../toggle'
import ErrorPanel from '../error-panel'
import SettingsPanel from '../settings-panel'
import ContactSupport from '../contact-support'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import getPanelBrowserAPI, {
  ConnectionState,
  REGION_PRECISION_COUNTRY
} from '../../api/panel_browser_api'
import Flag from '../flag'

const RegionInfo = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: flex-start;
  flex: 1 0 0;
`

const RegionLabel = styled.span`
  color: ${color.text.primary};
  font: ${font.heading.h4};
`

const RegionServerLabel = styled.span`
  color: ${color.text.secondary};
  font: ${font.small.regular};
`

function SessionExpiredContent() {
  const productUrls = useSelector((state) => state.productUrls)
  const message = getLocale('braveVpnSessionExpiredContent')

  const handleClick = (intent: string) => {
    if (!productUrls) return
    getPanelBrowserAPI().panelHandler.openVpnUI(intent)
  }

  return (
    <span>
      {formatMessage(message, {
        tags: {
          $1: (content) => (
            <a
              href='#'
              key='recoverAccount'
              onClick={() => handleClick('manage')}
            >
              {content}
            </a>
          )
        }
      })}
    </span>
  )
}

// Exported to share same header & settings button with loading panel.
export function PanelHeader(props: {
  title: string
  settingsTooltip: string
  settingsOnClick: () => void
}) {
  return (
    <S.PanelHeader>
      <S.VpnLogo name='product-vpn' />
      <S.PanelTitle>{props.title}</S.PanelTitle>
      <SettingsButton
        tooltip={props.settingsTooltip}
        onClick={props.settingsOnClick}
      />
    </S.PanelHeader>
  )
}

function SettingsButton(props: {
  tooltip: string
  onClick: () => void
}) {
  return (
    <S.SettingsButton
      type='button'
      onClick={props.onClick}
      title={props.tooltip}
    >
      <S.StyledIcon name='settings'></S.StyledIcon>
    </S.SettingsButton>
  )
}

function MainPanel() {
  const dispatch = useDispatch()
  const [isSettingsPanelVisible, setSettingsPanelVisible] =
    React.useState(false)
  const [isContactSupportVisible, setContactSupportVisible] =
    React.useState(false)
  const currentRegion = useSelector((state) => state.currentRegion)
  const hasError = useSelector((state) => state.hasError)
  const isSelectingRegion = useSelector((state) => state.isSelectingRegion)
  const connectionStatus = useSelector((state) => state.connectionStatus)
  const expired = useSelector((state) => state.expired)
  const outOfCredentials = useSelector((state) => state.outOfCredentials)
  const regions = useSelector((state) => state.regions)
  const stateDescription = useSelector((state) => state.stateDescription)

  const onSelectRegionButtonClick = () => {
    dispatch(Actions.toggleRegionSelector(true))
  }

  const handleSettingsButtonClick = () => setSettingsPanelVisible(true)
  const closeSettingsPanel = () => setSettingsPanelVisible(false)

  const showContactSupport = () => setContactSupportVisible(true)
  const closeContactSupport = () => setContactSupportVisible(false)

  const getCountryNameForCurrentRegion = () => {
    if (currentRegion.regionPrecision === REGION_PRECISION_COUNTRY) {
      return currentRegion.namePretty
    }

    for (const region of regions) {
      if (region.cities.find((city) => city.name === currentRegion.name)) {
        return region.namePretty
      }
    }

    return currentRegion.namePretty
  }

  if (isContactSupportVisible) {
    return <ContactSupport onCloseContactSupport={closeContactSupport} />
  }

  if (isSettingsPanelVisible) {
    return (
      <SettingsPanel
        closeSettingsPanel={closeSettingsPanel}
        showContactSupport={showContactSupport}
      />
    )
  }

  if (isSelectingRegion) {
    return <SelectRegionList />
  }

  if (hasError) {
    return <ErrorPanel showContactSupport={showContactSupport} />
  }

  const regionServerLabel =
    currentRegion.regionPrecision === REGION_PRECISION_COUNTRY
      ? getLocale('braveVpnServerSelectionOptimalLabel')
      : currentRegion.namePretty
  return (
    <PanelBox>
      <PanelHeader
        title={getLocale('braveVpn')}
        settingsTooltip={getLocale('braveVpnSettingsTooltip')}
        settingsOnClick={handleSettingsButtonClick}
      />
      <S.PanelContent>
        <Toggle disabled={expired} />
        {connectionStatus === ConnectionState.CONNECT_NOT_ALLOWED && (
          <S.StyledAlert
            type='warning'
            hideIcon
          >
            {getLocale('braveVpnConnectNotAllowed')}
          </S.StyledAlert>
        )}
        {expired && (
          <S.StyledAlert
            type='warning'
            hideIcon
          >
            <div slot='title'>{getLocale('braveVpnSessionExpiredTitle')}</div>
            <SessionExpiredContent />
          </S.StyledAlert>
        )}
        {outOfCredentials && (
          <S.StyledAlert
            type='warning'
            hideIcon
          >
            <div slot='title'>{getLocale('braveVpnOutOfCredentials')}</div>
            <div>{stateDescription}</div>
          </S.StyledAlert>
        )}
        {!outOfCredentials && (
          <S.RegionSelectorButton
            type='button'
            onClick={onSelectRegionButtonClick}
          >
            <Flag countryCode={currentRegion.countryIsoCode} />
            <RegionInfo>
              <RegionLabel>{getCountryNameForCurrentRegion()}</RegionLabel>
              <RegionServerLabel>{regionServerLabel}</RegionServerLabel>
            </RegionInfo>
            <S.StyledIcon name='carat-right' />
          </S.RegionSelectorButton>
        )}
      </S.PanelContent>
    </PanelBox>
  )
}

export default MainPanel
