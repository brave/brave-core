// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as Style from './style'
import { PanelHeader } from '../select-region-list'
import getPanelBrowserAPI, { ManageURLType } from '../../api/panel_browser_api'
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

  const [smartProxyRoutingEnabled, setSmartProxyRoutingEnabled] = React.useState({enabled: false})

  React.useEffect(() => {
    getPanelBrowserAPI().serviceHandler.getOnDemandState().then(setOnDemand)
    getPanelBrowserAPI().serviceHandler.getSmartProxyRoutingState().then(setSmartProxyRoutingEnabled)
  }, [])

  const handleClick = (entry: ManageURLType) => {
    getPanelBrowserAPI().panelHandler.openVpnUI(entry)
  }

  function handleKeyDown(
    entry: ManageURLType,
    event: React.KeyboardEvent<HTMLDivElement>
  ) {
    if (event.code !== 'Enter') {
      return
    }
    if (entry === ManageURLType.SUPPORT) {
      props.showContactSupport()
      return
    }
    handleClick(entry)
  }

  const handleToggleChange = ({ checked }: { checked: boolean }) => {
    setOnDemand({ ...onDemand, enabled: checked })
    getPanelBrowserAPI().serviceHandler.enableOnDemand(checked)
  }

  const handleSmartProxyRoutingChange = ({ checked }: { checked: boolean }) => {
    setSmartProxyRoutingEnabled({ enabled: checked })
    getPanelBrowserAPI().serviceHandler.enableSmartProxyRouting(checked)
  }

  return (
    <Style.Box>
      <Style.PanelContent>
        <PanelHeader
          title={getLocale('braveVpnSettingsPanelHeader')}
          buttonAriaLabel={
            getLocale('braveVpnSettingsPanelBackButtonAriaLabel')
          }
          onClick={props.closeSettingsPanel}
        />
        <Style.SettingsList>
          {onDemand.available && (
            <>
              <Style.Setting
                onClick={
                  e => handleToggleChange({ checked: !onDemand.enabled })
                }
              >
                <Style.SettingLabel>
                  {getLocale('braveVpnReconnectAutomatically')}
                </Style.SettingLabel>
                <Toggle
                  checked={onDemand.enabled}
                  onChange={handleToggleChange}
                  size='small'
                  aria-label='Reconnect automatically'
                />
              </Style.Setting>
              <Style.Divider />
            </>
          )}
          <Style.Setting
            onClick={
              e => handleSmartProxyRoutingChange({ checked: !smartProxyRoutingEnabled.enabled })
            }
          >
            <Style.SettingLabel>
              {getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING)}
            </Style.SettingLabel>
            <Toggle
              checked={smartProxyRoutingEnabled.enabled}
              onChange={handleSmartProxyRoutingChange}
              size='small'
              aria-label='Smart Proxy Routing'
            />
          </Style.Setting>
          <Style.Divider />
          <Style.Setting
            tabIndex={0}
            onClick={(e) => handleClick(ManageURLType.MANAGE)}
            onKeyDown={(e) => handleKeyDown(ManageURLType.MANAGE, e)}
          >
            <Style.SettingLabel>
              {getLocale('braveVpnManageSubscription')}
            </Style.SettingLabel>
            <Style.StyledIcon name='launch'></Style.StyledIcon>
          </Style.Setting>
          <Style.Divider />
          <Style.Setting
            tabIndex={0}
            onClick={props.showContactSupport}
            onKeyDown={(e) => handleKeyDown(ManageURLType.SUPPORT, e)}
          >
            <Style.SettingLabel>
              {getLocale('braveVpnContactSupport')}
            </Style.SettingLabel>
            <Style.StyledIcon name='carat-right'></Style.StyledIcon>
          </Style.Setting>
          <Style.Divider />
          <Style.Setting
            tabIndex={0}
            onClick={(e) => handleClick(ManageURLType.ABOUT)}
            onKeyDown={(e) => handleKeyDown(ManageURLType.ABOUT, e)}
          >
            <Style.SettingLabel>
              {getLocale('braveVpnAbout')} {getLocale('braveVpn')}
            </Style.SettingLabel>
            <Style.StyledIcon name='launch'></Style.StyledIcon>
          </Style.Setting>
        </Style.SettingsList>
      </Style.PanelContent>
    </Style.Box>
  )
}

export default SettingsPanel
