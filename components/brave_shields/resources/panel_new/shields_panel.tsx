/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { loadTimeData } from '$web-common/loadTimeData'
import { ShieldsPanelProxy } from './api/shields_panel_proxy'
import { createShieldsApi } from './api/shields_api'
import { ShieldsApiProvider } from './api/shields_api_context'
import { App } from './components/app'

setIconBasePath('//resources/brave-icons')

function loadFlag(key: string) {
  return loadTimeData.getBoolean(key)
}

function hasReloadsDetectedFlag() {
  const urlParams = new URLSearchParams(window.location.search)
  return urlParams.get('mode') === 'afterRepeatedReloads'
}

function createBrowserShieldsApi() {
  const proxy = ShieldsPanelProxy.getInstance()
  return createShieldsApi({
    dataHandler: proxy.dataHandler,
    panelHandler: proxy.panelHandler,
    createUIHandlerRemote: (impl) => proxy.createUIHandlerRemote(impl),
    openTab: (url) => chrome.tabs.create({ url, active: true }),
    loadTimeState: {
      isHttpsByDefaultEnabled: loadFlag('isHttpsByDefaultEnabled'),
      isTorProfile: loadFlag('isTorProfile'),
      showStrictFingerprintingMode: loadFlag('showStrictFingerprintingMode'),
      isWebcompatExceptionsServiceEnabled: loadFlag(
        'isWebcompatExceptionsServiceEnabled',
      ),
      isBraveForgetFirstPartyStorageFeatureEnabled: loadFlag(
        'isBraveForgetFirstPartyStorageFeatureEnabled',
      ),
      repeatedReloadsDetected: hasReloadsDetectedFlag(),
    },
  })
}

createRoot(document.getElementById('root')!).render(
  <ShieldsApiProvider api={createBrowserShieldsApi()}>
    <App />
  </ShieldsApiProvider>,
)
