// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as BraveVPN from '../../api/panel_browser_api'
import { PurchasedInfo } from '../../api/panel_browser_api'
import { mockRegionList } from './region-list'

const doNothing = () => {}

const loadingState = new PurchasedInfo();
loadingState.state = BraveVPN.PurchasedState.LOADING;

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
    initialize: doNothing,
    launchVPNPanel: doNothing,
    getPurchasedState: () => Promise.resolve({ state: loadingState }),
    getConnectionState: () => Promise.resolve({ state: BraveVPN.ConnectionState.CONNECTED }),
    resetConnectionState: doNothing,
    connect: doNothing,
    disconnect: doNothing,
    loadPurchasedState: doNothing,
    getAllRegions: () => Promise.resolve({ regions: mockRegionList }),
    getSelectedRegion: () => Promise.resolve({ currentRegion: mockRegionList[1] }),
    setSelectedRegion: doNothing,
    clearSelectedRegion: doNothing,
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
      hostname: 'site.example.com',
      timezone: 'USA/Boston'
    }),
    createSupportTicket: (email: string, subject: string, body: string) => Promise.resolve({
      success: true,
      response: 'OK'
    }),
    getOnDemandState: () => Promise.resolve({
      available: true,
      enabled: false
    }),
    enableOnDemand: (enable: boolean) => {}
  }
})
