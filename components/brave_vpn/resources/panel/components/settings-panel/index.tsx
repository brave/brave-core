// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import * as S from './style'
import { CaratStrongLeftIcon } from 'brave-ui/components/icons'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import Toggle from '$web-components/toggle'

interface Props {
  closeSettingsPanel: React.MouseEventHandler<HTMLButtonElement>
  showContactSupport: () => void
}

function SettingsPanel (props: Props) {
  const [onDemand, setOnDemand] =
    React.useState({ available: false, enabled: false})

  getPanelBrowserAPI().serviceHandler.getOnDemandState().then(setOnDemand)

  const handleClick = (entry: string) => {
    getPanelBrowserAPI().panelHandler.openVpnUI(entry)
  }
  const handleKeyDown = (entry: string, event: React.KeyboardEvent<HTMLAnchorElement>) => {
    if (event.keyCode !== 32) { return }
    if (entry === 'support') {
      props.showContactSupport()
      return
    }
    handleClick(entry)
  }
  const handleToggleChange = (isOn: boolean) => {
    getPanelBrowserAPI().serviceHandler.enableOnDemand(isOn)
  }

  return (
    <S.Box>
      <S.PanelContent>
        <S.PanelHeader>
          <S.BackButton
            type='button'
            onClick={props.closeSettingsPanel}
            aria-label='Close settings'
          >
            <i><CaratStrongLeftIcon /></i>
            <span>{getLocale('braveVpnSettingsPanelHeader')}</span>
          </S.BackButton>
        </S.PanelHeader>
        <S.List>
          {onDemand.available && <li>
            <S.ReconnectBox>
              <span>
                {getLocale('braveVpnReconnectAutomatically')}
              </span>
              <Toggle
                isOn={onDemand.enabled}
                onChange={handleToggleChange}
                brand='vpn'
                size='sm'
                aria-label='Reconnect automatically'
              />
            </S.ReconnectBox>
          </li>}
          <li>
            <a href="#" onClick={handleClick.bind(this, 'manage')} onKeyDown={handleKeyDown.bind(this, 'manage')}>
              {getLocale('braveVpnManageSubscription')}
            </a>
          </li>
          <li>
            <a href="#" onClick={props.showContactSupport} onKeyDown={handleKeyDown.bind(this, 'support')}>
              {getLocale('braveVpnContactSupport')}
            </a>
          </li>
          <li>
            <a href="#" onClick={handleClick.bind(this, 'about')} onKeyDown={handleKeyDown.bind(this, 'about')}>
              {getLocale('braveVpnAbout')}
              {' '}
              {getLocale('braveVpn')}
            </a>
          </li>
        </S.List>
      </S.PanelContent>
    </S.Box>
  )
}

export default SettingsPanel
