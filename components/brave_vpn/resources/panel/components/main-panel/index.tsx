// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import * as Styles from './style'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import { color, font } from '@brave/leo/tokens/css/variables'
import { getLocale, formatLocale } from '$web-common/locale'
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
  ManageURLType,
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

const SmartProxyIcon = styled(Icon)`
  --leo-icon-size: 18px;
  --leo-icon-color: ${color.icon.default};
  background: ${color.container.highlight};
`

const RegionLabelBox = styled.span`
  display: flex;
  align-items: center;
  gap: 10px;
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

  const handleClick = (intent: ManageURLType) => {
    if (!productUrls) return
    getPanelBrowserAPI().panelHandler.openVpnUI(intent)
  }

  return (
    <span>
      {formatLocale('braveVpnSessionExpiredContent', {
        $1: (content) => (
          <a
            href='#'
            onClick={() => handleClick(ManageURLType.MANAGE)}
          >
            {content}
          </a>
        )
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
    <Styles.PanelHeader>
      <Styles.VpnLogo name='product-vpn' />
      <Styles.PanelTitle>{props.title}</Styles.PanelTitle>
      <SettingsButton
        tooltip={props.settingsTooltip}
        onClick={props.settingsOnClick}
      />
    </Styles.PanelHeader>
  )
}

function SettingsButton(props: {
  tooltip: string
  onClick: () => void
}) {
  return (
    <Styles.SettingsButton
      type='button'
      onClick={props.onClick}
      title={props.tooltip}
    >
      <Styles.StyledIcon name='settings'></Styles.StyledIcon>
    </Styles.SettingsButton>
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
  const smartProxyRoutingEnabled = useSelector((state) => state.smartProxyRoutingEnabled)

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
      ? getLocale(S.BRAVE_VPN_SERVER_SELECTION_OPTIMAL_LABEL)
      : currentRegion.namePretty
  return (
    <PanelBox>
      <PanelHeader
        title={getLocale(S.BRAVE_VPN)}
        settingsTooltip={getLocale(S.BRAVE_VPN_MAIN_PANEL_VPN_SETTINGS_TITLE)}
        settingsOnClick={handleSettingsButtonClick}
      />
      <Styles.PanelContent>
        <Toggle disabled={expired} />
        {connectionStatus === ConnectionState.CONNECT_NOT_ALLOWED && (
          <Styles.StyledAlert
            type='warning'
            hideIcon
          >
            {getLocale(S.BRAVE_VPN_CONNECT_NOT_ALLOWED)}
          </Styles.StyledAlert>
        )}
        {expired && (
          <Styles.StyledAlert
            type='warning'
            hideIcon
          >
            <div slot='title'>{getLocale(S.BRAVE_VPN_MAIN_PANEL_SESSION_EXPIRED_PART_TITLE)}</div>
            <SessionExpiredContent />
          </Styles.StyledAlert>
        )}
        {outOfCredentials && (
          <Styles.StyledAlert
            type='warning'
            hideIcon
          >
            <div slot='title'>{getLocale(S.BRAVE_VPN_MAIN_PANEL_OUT_OF_CREDENTIALS_TITLE)}</div>
            <div>{stateDescription}</div>
          </Styles.StyledAlert>
        )}
        {!outOfCredentials && (
          <Styles.RegionSelectorButton
            type='button'
            onClick={onSelectRegionButtonClick}
          >
            <Flag countryCode={currentRegion.countryIsoCode} />
            <RegionInfo>
              <RegionLabelBox>
                <RegionLabel>{getCountryNameForCurrentRegion()}</RegionLabel>
                {smartProxyRoutingEnabled && currentRegion.smartRoutingProxyState === 'all' && (
                  <Tooltip mode='mini'>
                    <SmartProxyIcon name='smart-proxy-routing' />
                    <div slot='content'>
                      {getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING_ICON_TOOLTIP)}
                    </div>
                  </Tooltip>)
                }
              </RegionLabelBox>
              <RegionServerLabel>{regionServerLabel}</RegionServerLabel>
            </RegionInfo>
            <Styles.StyledIcon name='carat-right' />
          </Styles.RegionSelectorButton>
        )}
      </Styles.PanelContent>
    </PanelBox>
  )
}

export default MainPanel
