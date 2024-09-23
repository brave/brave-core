// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { withKnobs } from '@storybook/addon-knobs'
import { VPNWidget } from '../components/default/vpn/vpn_card';
import * as BraveVPN from '../api/braveVpn';
import { createStore } from 'redux';
import { Provider } from 'react-redux';

export default {
  title: 'VPN/Widgets',
  decorators: [
    (Story: any) => {
      const store = createStore(state => state)
      return <Provider store={store}><Story /></Provider>
    },
    withKnobs
  ]
}

const vpnWidgetProps = {
  purchasedState: BraveVPN.PurchasedState.NOT_PURCHASED,
  connectionState: BraveVPN.ConnectionState.DISCONNECTED,
  selectedRegion: new BraveVPN.Region()
}

export const VPN = () => (
  <VPNWidget {...vpnWidgetProps} />
)
