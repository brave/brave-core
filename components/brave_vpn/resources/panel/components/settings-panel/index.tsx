// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import { PanelHeader } from '../select-region-list'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { getLocale } from '$web-common/locale'
import Toggle from '@brave/leo/react/toggle'

interface Props {
  closeSettingsPanel: () => void
  showContactSupport: () => void
}

function SettingsPanel(props: Props) {
  const [onDemand, setOnDemand] = React.useState({
    available: false,
    enabled: false
  })

  React.useEffect(() => {
    getPanelBrowserAPI().serviceHandler.getOnDemandState().then(setOnDemand)
  }, [])

  const handleClick = (entry: string) => {
    getPanelBrowserAPI().panelHandler.openVpnUI(entry)
  }

  function handleKeyDown(
    entry: string,
    event: React.KeyboardEvent<HTMLDivElement>
  ) {
    if (event.code !== 'Enter') {
      return
    }
    if (entry === 'support') {
      props.showContactSupport()
      return
    }
    handleClick(entry)
  }

  const handleToggleChange = ({ checked }: { checked: boolean }) => {
    setOnDemand({ ...onDemand, enabled: checked })
    getPanelBrowserAPI().serviceHandler.enableOnDemand(checked)
  }

  return (
    <S.Box>
      <S.PanelContent>
        <PanelHeader
          title={getLocale('braveVpnSettingsPanelHeader')}
          buttonAriaLabel={
            getLocale('braveVpnSettingsPanelBackButtonAriaLabel')
          }
          onClick={props.closeSettingsPanel}
        />
        <S.SettingsList>
          {onDemand.available && (
            <>
              <S.Setting
                onClick={
                  e => handleToggleChange({ checked: !onDemand.enabled })
                }
              >
                <S.SettingLabel>
                  {getLocale('braveVpnReconnectAutomatically')}
                </S.SettingLabel>
                <Toggle
                  checked={onDemand.enabled}
                  onChange={handleToggleChange}
                  size='small'
                  aria-label='Reconnect automatically'
                />
              </S.Setting>
              <S.Divider />
            </>
          )}
          <S.Setting tabIndex={0} onClick={e => handleClick('manage')}
            onKeyDown={e => handleKeyDown('manage', e)}
          >
            <S.SettingLabel>
              {getLocale('braveVpnManageSubscription')}
            </S.SettingLabel>
            <S.StyledIcon name='launch'></S.StyledIcon>
          </S.Setting>
          <S.Divider />
          <S.Setting tabIndex={0} onClick={props.showContactSupport}
            onKeyDown={e => handleKeyDown('support', e)}
          >
            <S.SettingLabel>
              {getLocale('braveVpnContactSupport')}
            </S.SettingLabel>
            <S.StyledIcon name='carat-right'></S.StyledIcon>
          </S.Setting>
          <S.Divider />
          <S.Setting tabIndex={0} onClick={e => handleClick('about')}
            onKeyDown={e => handleKeyDown('about', e)}
          >
            <S.SettingLabel>
              {getLocale('braveVpnAbout')} {getLocale('braveVpn')}
            </S.SettingLabel>
            <S.StyledIcon name='launch'></S.StyledIcon>
          </S.Setting>
        </S.SettingsList>
      </S.PanelContent>
    </S.Box>
  )
}

export default SettingsPanel
