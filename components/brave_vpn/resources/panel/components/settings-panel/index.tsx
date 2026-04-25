// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as Styles from './style'
import { PanelHeader } from '../select-region-list'
import getPanelBrowserAPI, { ManageURLType } from '../../api/panel_browser_api'
import { getLocale } from '$web-common/locale'
import Toggle from '@brave/leo/react/toggle'
import { useSelector } from '../../state/hooks'

interface Props {
  closeSettingsPanel: () => void
  showContactSupport: () => void
}

function SettingsPanel(props: Props) {
  const [onDemand, setOnDemand] = React.useState({
    available: false,
    enabled: false
  })

  const smartProxyRoutingEnabled = useSelector((state) => state.smartProxyRoutingEnabled)

  React.useEffect(() => {
    getPanelBrowserAPI().serviceHandler.getOnDemandState().then(setOnDemand)
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
    getPanelBrowserAPI().serviceHandler.enableSmartProxyRouting(checked)
  }

  return (
    <Styles.Box>
      <Styles.PanelContent>
        <PanelHeader
          title={getLocale(S.BRAVE_VPN_SETTINGS_PANEL_HEADER)}
          buttonAriaLabel={
            getLocale(S.BRAVE_VPN_SETTINGS_PANEL_BACK_BUTTON_ARIA_LABEL)
          }
          onClick={props.closeSettingsPanel}
        />
        <Styles.Divider />
        <Styles.SettingsList>
          {onDemand.available && (
            <>
              <Styles.Setting
                onClick={
                  e => handleToggleChange({ checked: !onDemand.enabled })
                }
              >
                <Styles.StyledIcon name='refresh'></Styles.StyledIcon>
                <Styles.SettingLabelBox>
                  <Styles.SettingLabel>
                    {getLocale(S.BRAVE_VPN_RECONNECT_AUTOMATICALLY)}
                  </Styles.SettingLabel>
                </Styles.SettingLabelBox>
                <Toggle
                  checked={onDemand.enabled}
                  onChange={handleToggleChange}
                  size='small'
                  aria-label='Reconnect automatically'
                />
              </Styles.Setting>
            </>
          )}
          <Styles.Setting
            onClick={
              e => handleSmartProxyRoutingChange({ checked: !smartProxyRoutingEnabled })
            }
          >
            <Styles.StyledIcon name='smart-proxy-routing'></Styles.StyledIcon>
            <Styles.SettingLabelBox>
              <Styles.SettingLabel>
                {getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING)}
              </Styles.SettingLabel>
              <Styles.SettingDesc>
                {getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING_DESC)}{' '}
                  <a
                    href='#'
                    onClick={
                      (e) => {
                        // Prevent toggle when clicking this link.
                        e.stopPropagation()
                        handleClick(ManageURLType.ABOUT_SMART_PROXY)
                      }
                    }
                  >
                    {getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING_DESC_LEARN_MORE)}
                  </a>
              </Styles.SettingDesc>
            </Styles.SettingLabelBox>
            <Toggle
              checked={smartProxyRoutingEnabled}
              onChange={handleSmartProxyRoutingChange}
              size='small'
              area-label={getLocale(S.BRAVE_VPN_SMART_PROXY_ROUTING)}
            />
          </Styles.Setting>
          <Styles.Setting
            tabIndex={0}
            onClick={(e) => handleClick(ManageURLType.MANAGE)}
            onKeyDown={(e) => handleKeyDown(ManageURLType.MANAGE, e)}
          >
            <Styles.StyledIcon name='premium-indicator'></Styles.StyledIcon>
            <Styles.SettingLabelBox>
              <Styles.SettingLabel>
                {getLocale(S.BRAVE_VPN_MANAGE_SUBSCRIPTION)}
              </Styles.SettingLabel>
            </Styles.SettingLabelBox>
            <Styles.StyledIcon name='launch'></Styles.StyledIcon>
          </Styles.Setting>
          <Styles.Setting
            tabIndex={0}
            onClick={props.showContactSupport}
            onKeyDown={(e) => handleKeyDown(ManageURLType.SUPPORT, e)}
          >
            <Styles.StyledIcon name='support'></Styles.StyledIcon>
            <Styles.SettingLabelBox>
              <Styles.SettingLabel>
                {getLocale(S.BRAVE_VPN_CONTACT_SUPPORT)}
              </Styles.SettingLabel>
            </Styles.SettingLabelBox>
            <Styles.StyledIcon name='carat-right'></Styles.StyledIcon>
          </Styles.Setting>
          <Styles.Setting
            tabIndex={0}
            onClick={(e) => handleClick(ManageURLType.ABOUT)}
            onKeyDown={(e) => handleKeyDown(ManageURLType.ABOUT, e)}
          >
            <Styles.StyledIcon name='product-vpn'></Styles.StyledIcon>
            <Styles.SettingLabelBox>
              <Styles.SettingLabel>
                {getLocale(S.BRAVE_VPN_ABOUT)} {getLocale(S.BRAVE_VPN)}
              </Styles.SettingLabel>
            </Styles.SettingLabelBox>
            <Styles.StyledIcon name='launch'></Styles.StyledIcon>
          </Styles.Setting>
        </Styles.SettingsList>
      </Styles.PanelContent>
    </Styles.Box >
  )
}

export default SettingsPanel
