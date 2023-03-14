// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SettingsAdvancedIcon, CaratStrongRightIcon } from 'brave-ui/components/icons'

import * as S from './style'
import { getLocale } from '../../../../../common/locale'
import { formatMessage } from '../../../../../brave_rewards/resources/shared/lib/locale_context'
import SelectRegionList from '../select-region-list'
import PanelBox from '../panel-box'
import Toggle from '../toggle'
import ErrorPanel from '../error-panel'
import SettingsPanel from '../settings-panel'
import ContactSupport from '../contact-support'
import { useSelector, useDispatch } from '../../state/hooks'
import * as Actions from '../../state/actions'
import getPanelBrowserAPI, { ConnectionState } from '../../api/panel_browser_api'
import Flag from '../flag'

function SessionExpiredContent () {
  const productUrls = useSelector(state => state.productUrls)
  const message = getLocale('braveVpnSessionExpiredContent')

  const handleClick = (intent: string) => {
    if (!productUrls) return
    getPanelBrowserAPI().panelHandler.openVpnUI(intent)
  }

  return (
    <span>
      {
        formatMessage(message, {
          tags: {
            $1: (content) => (
              <a href="#" key="recoverAccount" onClick={handleClick.bind(null, 'manage')}>
                {content}
              </a>
            )
          }
        })
      }
    </span>
  )
}

function MainPanel () {
  const dispatch = useDispatch()
  const [isSettingsPanelVisible, setSettingsPanelVisible] = React.useState(false)
  const [isContactSupportVisible, setContactSupportVisible] = React.useState(false)
  const currentRegion = useSelector(state => state.currentRegion)
  const hasError = useSelector(state => state.hasError)
  const isSelectingRegion = useSelector(state => state.isSelectingRegion)
  const connectionStatus = useSelector(state => state.connectionStatus)
  const expired = useSelector(state => state.expired)

  const onSelectRegionButtonClick = () => {
    dispatch(Actions.toggleRegionSelector(true))
  }

  const handleSettingsButtonClick = () => setSettingsPanelVisible(true)
  const closeSettingsPanel = () => setSettingsPanelVisible(false)

  const showContactSupport = () => setContactSupportVisible(true)
  const closeContactSupport = () => setContactSupportVisible(false)

  if (isContactSupportVisible) {
    return (<ContactSupport
      onCloseContactSupport={closeContactSupport}
    />)
  }

  if (isSettingsPanelVisible) {
    return (<SettingsPanel
      closeSettingsPanel={closeSettingsPanel}
      showContactSupport={showContactSupport}
    />)
  }

  if (isSelectingRegion) {
    return (<SelectRegionList />)
  }

  if (hasError) {
    return (<ErrorPanel showContactSupport={showContactSupport} />)
  }

  return (
    <PanelBox>
      <S.PanelContent>
        <S.PanelHeader>
          <S.SettingsButton
            type='button'
            onClick={handleSettingsButtonClick}
            title={getLocale('braveVpnSettingsTooltip')}
          >
            <SettingsAdvancedIcon />
          </S.SettingsButton>
        </S.PanelHeader>
        <S.PanelTitle>{getLocale('braveVpn')}</S.PanelTitle>
        <Toggle disabled={expired} />
        {connectionStatus === ConnectionState.CONNECT_NOT_ALLOWED && (
          <S.ConnectNotAllowedNote>
          <div>{getLocale('braveVpnConnectNotAllowed')}</div>
          </S.ConnectNotAllowedNote>
        )}
        {expired && (
          <S.SessionExpiredNote>
            <S.SessionExpiredNoteTitle>
              {getLocale('braveVpnSessionExpiredTitle')}
            </S.SessionExpiredNoteTitle>
            <SessionExpiredContent />
          </S.SessionExpiredNote>
        )}
        <S.RegionSelectorButton
          type='button'
          onClick={onSelectRegionButtonClick}
        >
          <Flag countryCode={currentRegion?.countryIsoCode}/>
          <S.RegionLabel>{currentRegion?.namePretty}</S.RegionLabel>
          <CaratStrongRightIcon />
        </S.RegionSelectorButton>
      </S.PanelContent>
    </PanelBox>
  )
}

export default MainPanel
