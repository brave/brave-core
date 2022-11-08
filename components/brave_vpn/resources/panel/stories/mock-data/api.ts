// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as BraveVPN from '../../api/panel_browser_api'
import { mockRegionList } from './region-list'

const doNothing = () => {}

BraveVPN.setPanelBrowserApiForTesting({
  pageCallbackRouter: {
  },
  panelHandler: {
    showUI: doNothing,
    closeUI: doNothing,
    openVpnUI: doNothing
  },
  serviceHandler: {
    addObserver: doNothing,
    getPurchasedState: () => Promise.resolve({ state: BraveVPN.PurchasedState.LOADING }),
    getConnectionState: () => Promise.resolve({ state: BraveVPN.ConnectionState.CONNECTED }),
    connect: doNothing,
    disconnect: doNothing,
    loadPurchasedState: doNothing,
    getAllRegions: () => Promise.resolve({ regions: mockRegionList }),
    getSelectedRegion: () => Promise.resolve({ currentRegion: mockRegionList[1] }),
    setSelectedRegion: doNothing,
    getProductUrls: () => Promise.resolve({
      urls: {
        feedback: 'https://example.com',
        about: 'https://about.example.com',
        manage: 'https://manage.example.com'
      }
    }),
    getSupportData: () => Promise.resolve({
      appVersion: '1',
      osVersion: '2',
      hostname: 'site.example.com'
    }),
    createSupportTicket: (email: string, subject: string, body: string) => Promise.resolve({
      success: true,
      response: 'OK'
    })
  }
})
