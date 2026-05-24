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

function hasReloadsDetectedFlag() {
  const urlParams = new URLSearchParams(window.location.search)
  return urlParams.get('mode') === 'afterRepeatedReloads'
}

function getSettingsPagePath(url: string) {
  try {
    const parsedUrl = new URL(url)
    if (
      (parsedUrl.protocol === 'chrome:' || parsedUrl.protocol === 'brave:') &&
      parsedUrl.hostname === 'settings'
    ) {
      return parsedUrl.pathname
    }
  } catch {
    return null
  }

  return null
}

function isSettingsPageTab(tab: chrome.tabs.Tab, pagePath: string) {
  const tabUrl = tab.pendingUrl ?? tab.url
  if (!tabUrl) {
    return false
  }

  try {
    const parsedUrl = new URL(tabUrl)
    return (
      (parsedUrl.protocol === 'chrome:' || parsedUrl.protocol === 'brave:') &&
      parsedUrl.hostname === 'settings' &&
      parsedUrl.pathname === pagePath
    )
  } catch {
    return false
  }
}

function openOrFocusTab(url: string) {
  const settingsPagePath = getSettingsPagePath(url)
  if (settingsPagePath !== '/shields/filters') {
    chrome.tabs.create({ url, active: true })
    return
  }

  chrome.tabs.query({}, (tabs) => {
    if (chrome.runtime.lastError) {
      chrome.tabs.create({ url, active: true })
      return
    }

    const existingTab = tabs.find((tab) => {
      return isSettingsPageTab(tab, settingsPagePath)
    })
    if (existingTab?.id === undefined) {
      chrome.tabs.create({ url, active: true })
      return
    }

    if (existingTab.windowId !== undefined) {
      chrome.windows.update(existingTab.windowId, { focused: true })
    }

    chrome.tabs.update(existingTab.id, { active: true })
  })
}

function createBrowserShieldsApi() {
  const proxy = ShieldsPanelProxy.getInstance()
  return createShieldsApi({
    dataHandler: proxy.dataHandler,
    panelHandler: proxy.panelHandler,
    createUIHandlerRemote: (impl) => proxy.createUIHandlerRemote(impl),
    openTab,
    loadTimeState: {
      isHttpsByDefaultEnabled: loadTimeData.getBoolean(
        'isHttpsByDefaultEnabled',
      ),
      isTorProfile: loadTimeData.getBoolean('isTorProfile'),
      showStrictFingerprintingMode: loadTimeData.getBoolean(
        'showStrictFingerprintingMode',
      ),
      isWebcompatExceptionsServiceEnabled: loadTimeData.getBoolean(
        'isWebcompatExceptionsServiceEnabled',
      ),
      isBraveForgetFirstPartyStorageFeatureEnabled: loadTimeData.getBoolean(
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
