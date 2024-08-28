// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
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
export function PanelHeader(props: { title: string }) {
  return (
    <S.PanelHeader>
      <S.VpnLogo name='product-vpn' />
      <S.PanelTitle>{props.title}</S.PanelTitle>
    </S.PanelHeader>
  )
}

export function SettingsButton(props: {
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
  const regions = useSelector((state) => state.regions)

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

    for (let i = 0; i < regions.length; ++i) {
      if (regions[i].cities.find((city) => city.name === currentRegion.name)) {
        return regions[i].namePretty
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
      <S.PanelContent>
        <SettingsButton
          tooltip={getLocale('braveVpnSettingsTooltip')}
          onClick={handleSettingsButtonClick}
        />
        <PanelHeader title={getLocale('braveVpnMainPanelTitle')} />
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
            mode='full'
            hideIcon
          >
            <div slot='title'>{getLocale('braveVpnSessionExpiredTitle')}</div>
            <SessionExpiredContent />
          </S.StyledAlert>
        )}
        <S.RegionSelectorButton
          type='button'
          onClick={onSelectRegionButtonClick}
        >
          <Flag countryCode={currentRegion.countryIsoCode} />
          <S.RegionInfo>
            <S.RegionLabel>{getCountryNameForCurrentRegion()}</S.RegionLabel>
            <S.RegionServerLabel>{regionServerLabel}</S.RegionServerLabel>
          </S.RegionInfo>
          <S.StyledIcon name='carat-right' />
        </S.RegionSelectorButton>
      </S.PanelContent>
    </PanelBox>
  )
}

export default MainPanel
