// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { createStore } from 'redux'
import { select, withKnobs } from '@storybook/addon-knobs'
import {
  VPNPromoWidget,
  VPNMainWidget
} from '../components/default/vpn/vpn_card'
import * as BraveVPN from '../api/braveVpn'
import '../../brave_vpn/resources/panel/stories/locale'
import {
  mockRegionList
} from '../../brave_vpn/resources/panel/stories/mock-data/region-list'

export default {
  title: 'VPN/Card',
  decorators: [
    (Story: any) => {
      const store = createStore((state) => state)
      return (
        <Provider store={store}>
          <Story />
        </Provider>
      )
    },
    withKnobs
  ]
}

export const VPNPromo = () => <VPNPromoWidget />

export const VPNMain = () => (
  <VPNMainWidget
    connectionState={select(
      'connection state',
      BraveVPN.ConnectionState,
      BraveVPN.ConnectionState.DISCONNECTED
    )}
    selectedRegion={mockRegionList[0]}
  />
)
