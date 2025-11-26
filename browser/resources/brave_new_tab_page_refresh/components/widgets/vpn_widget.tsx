/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { getString } from '../../lib/strings'
import { ConnectionState } from '../../state/vpn_state'
import { useVpnState, useVpnActions } from '../../context/vpn_context'
import { WidgetMenu } from './widget_menu'
import classNames from '$web-common/classnames'

import { style } from './vpn_widget.style'

import guardianLogoURL from '../../assets/guardian_vpn_logo.svg'
import vpnShieldDisconnectedURL from '../../assets/vpn_shield_disconnected.svg'
import vpnShieldConnectedURL from '../../assets/vpn_shield_connected.svg'

export function VpnWidget() {
  const actions = useVpnActions()

  const purchased = useVpnState((s) => s.vpnPurchased)
  const connectionState = useVpnState((s) => s.vpnConnectionState)
  const region = useVpnState((s) => s.vpnConnectionRegion)

  function renderPromo() {
    return (
      <VpnWidgetContainer>
        <div className='title'>
          <span>
            <Icon name='product-vpn' />
            {getString(S.NEW_TAB_VPN_WIDGET_TITLE)}
          </span>
          <span className='provider'>
            {getString(S.NEW_TAB_VPN_POWERED_BY_TEXT)}
            <img src={guardianLogoURL} />
          </span>
        </div>
        <div className='content'>
          <div className='features'>
            <div>
              <Icon name='shield-done' />
              <span>{getString(S.NEW_TAB_VPN_FEATURE_TEXT1)}</span>
            </div>
            <div>
              <Icon name='shield-done' />
              <span>{getString(S.NEW_TAB_VPN_FEATURE_TEXT2)}</span>
            </div>
            <div>
              <Icon name='shield-done' />
              <span>{getString(S.NEW_TAB_VPN_FEATURE_TEXT3)}</span>
            </div>
          </div>
          <div className='purchase-actions'>
            <Button
              size='small'
              onClick={() => actions.startVpnTrial()}
            >
              {getString(S.NEW_TAB_VPN_START_TRIAL_LABEL)}
            </Button>
            <button
              className='restore'
              onClick={() => actions.restoreVpnPurchase()}
            >
              {getString(S.NEW_TAB_VPN_RESTORE_PURCHASE_LABEL)}
            </button>
          </div>
        </div>
      </VpnWidgetContainer>
    )
  }

  function renderConnectionGraphic() {
    const image =
      connectionState === ConnectionState.CONNECTED
        ? vpnShieldConnectedURL
        : vpnShieldDisconnectedURL

    return <img src={image} />
  }

  function connectionStateText() {
    switch (connectionState) {
      case ConnectionState.CONNECTED:
        return getString(S.NEW_TAB_VPN_STATUS_CONNECTED)
      case ConnectionState.DISCONNECTED:
        return getString(S.NEW_TAB_VPN_STATUS_DISCONNECTED)
      case ConnectionState.CONNECTING:
        return getString(S.NEW_TAB_VPN_STATUS_CONNECTING)
      case ConnectionState.DISCONNECTING:
        return getString(S.NEW_TAB_VPN_STATUS_DISCONNECTING)
      default:
        console.error('Unhandled ConnectionState', connectionState)
        return ''
    }
  }

  function renderRegionInfo() {
    if (!region) {
      return null
    }

    return (
      <div className='region'>
        <div className='country'>
          {region.country}
          <button onClick={() => actions.openVpnPanel()}>
            {getString(S.NEW_TAB_VPN_CHANGE_REGION_LABEL)}
          </button>
        </div>
        <div>
          {region.namePretty === region.country
            ? getString(S.NEW_TAB_VPN_OPTIMAL_TEXT)
            : region.namePretty}
        </div>
      </div>
    )
  }

  if (!purchased) {
    return renderPromo()
  }

  return (
    <VpnWidgetContainer>
      <div className='title'>{getString(S.NEW_TAB_VPN_WIDGET_TITLE)}</div>
      <div
        className={classNames({
          'content': true,
          'connected': connectionState === ConnectionState.CONNECTED,
        })}
      >
        <div className='connection-graphic'>{renderConnectionGraphic()}</div>
        <div className='connection-info'>
          <div className='status'>{connectionStateText()}</div>
          {renderRegionInfo()}
        </div>
        <div className='connection-toggle'>
          <Toggle
            checked={
              connectionState === ConnectionState.CONNECTED
              || connectionState === ConnectionState.CONNECTING
            }
            onChange={() => actions.toggleVpnConnection()}
          />
        </div>
      </div>
    </VpnWidgetContainer>
  )
}

function VpnWidgetContainer(props: React.PropsWithChildren) {
  const actions = useVpnActions()
  return (
    <div data-css-scope={style.scope}>
      <WidgetMenu>
        <leo-menu-item onClick={() => actions.setShowVpnWidget(false)}>
          <Icon name='eye-off' /> {getString(S.NEW_TAB_HIDE_VPN_WIDGET_LABEL)}
        </leo-menu-item>
      </WidgetMenu>
      {props.children}
    </div>
  )
}
