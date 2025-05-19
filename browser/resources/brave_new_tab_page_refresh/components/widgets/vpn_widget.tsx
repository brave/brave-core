/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useAppActions, useAppState } from '../context/app_model_context'
import { getString } from '../../lib/strings'
import { ConnectionState } from '../../models/vpn'
import classNames from '$web-common/classnames'

import { style } from './vpn_widget.style'

import guardianLogoURL from '../../assets/guardian_vpn_logo.svg'
import vpnShieldDisconnectedURL from '../../assets/vpn_shield_disconnected.svg'
import vpnShieldConnectedURL from '../../assets/vpn_shield_connected.svg'

export function VPNWidget() {
  const actions = useAppActions()

  const purchased = useAppState((s) => s.vpnPurchased)
  const connectionState = useAppState((s) => s.vpnConnectionState)
  const region = useAppState((s) => s.vpnConnectionRegion)

  function renderPromo() {
    return (
      <div data-css-scope={style.scope}>
        <div className='title'>
          <span>
            <Icon name='product-vpn' />
            {getString('vpnWidgetTitle')}
          </span>
          <span className='provider'>
            {getString('vpnPoweredByText')} <img src={guardianLogoURL} />
          </span>
        </div>
        <div className='content'>
          <div className='features'>
            <div>
              <Icon name='shield-done' />
              <span>{getString('vpnFeatureText1')}</span>
            </div>
            <div>
              <Icon name='shield-done' />
              <span>{getString('vpnFeatureText2')}</span>
            </div>
            <div>
              <Icon name='shield-done' />
              <span>{getString('vpnFeatureText3')}</span>
            </div>
          </div>
          <div className='purchase-actions'>
            <Button
              size='small'
              onClick={() => actions.startVpnTrial()}
            >
              {getString('vpnStartTrialLabel')}
            </Button>
            <button
              className='restore'
              onClick={() => actions.restoreVpnPurchase()}
            >
              {getString('vpnRestorePurchaseLabel')}
            </button>
          </div>
        </div>
      </div>
    )
  }

  function renderConnectionGraphic() {
    const image = connectionState === ConnectionState.CONNECTED
      ? vpnShieldConnectedURL
      : vpnShieldDisconnectedURL

    return <img src={image} />
  }

  function connectionStateText() {
    switch (connectionState) {
      case ConnectionState.CONNECTED:
        return getString('vpnStatusConnected')
      case ConnectionState.DISCONNECTED:
        return getString('vpnStatusDisconnected')
      case ConnectionState.CONNECTING:
        return getString('vpnStatusConnecting')
      case ConnectionState.DISCONNECTING:
        return getString('vpnStatusDisconnecting')
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
            {getString('vpnChangeRegionLabel')}
          </button>
        </div>
        <div>
          {
            region.namePretty === region.country
              ? getString('vpnOptimalText')
              : region.namePretty
          }
        </div>
      </div>
    )
  }

  if (!purchased) {
    return renderPromo()
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='title'>{getString('vpnWidgetTitle')}</div>
      <div
        className={classNames({
          'content': true,
          'connected': connectionState === ConnectionState.CONNECTED
        })}>
        <div className='connection-graphic'>
          {renderConnectionGraphic()}
        </div>
        <div className='connection-info'>
          <div className='status'>{connectionStateText()}</div>
          {renderRegionInfo()}
        </div>
        <div className='connection-toggle'>
          <Toggle
            checked={
              connectionState === ConnectionState.CONNECTED ||
              connectionState === ConnectionState.CONNECTING
            }
            onChange={() => actions.toggleVpnConnection()}
          />
        </div>
      </div>
    </div>
  )
}
