/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { useAppActions, useAppState } from '../context/app_model_context'
import { useLocale } from '../context/locale_context'

import {
  style,
  guardianLogoURL,
  vpnShieldConnectedURL,
  vpnShieldDisconnectedURL } from './vpn_widget.style'

export function VPNWidget() {
  const { getString } = useLocale()
  const actions = useAppActions()

  const [purchased, connectionState, region] = useAppState((state) => [
    state.vpnPurchased,
    state.vpnConnectionState,
    state.vpnConnectionRegion
  ])

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
    const image = connectionState === 'connected'
      ? vpnShieldConnectedURL
      : vpnShieldDisconnectedURL

    return <img src={image} />
  }

  function connectionStateText() {
    switch (connectionState) {
      case 'connected': return getString('vpnStatusConnected')
      case 'disconnected': return getString('vpnStatusDisconnected')
      case 'connecting': return getString('vpnStatusConnecting')
      case 'disconnecting': return getString('vpnStatusDisconnecting')
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
            region.name === region.country
              ? getString('vpnOptimalText')
              : region.name
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
      <div className={`content ${connectionState}`}>
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
              connectionState === 'connected' ||
              connectionState === 'connecting'
            }
            onChange={() => actions.toggleVpnConnection()}
          />
        </div>
      </div>
    </div>
  )
}
