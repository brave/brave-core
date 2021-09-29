// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { App } from '../../../../../vendor/web-discovery-project/build'

declare var window: any

function onCommitted (details: chrome.webNavigation.WebNavigationTransitionCallbackDetails) {
  // Only inject if page is acceptable protocol (to skip internal pages like
  // devtools://, brave://, etc.). We also make sure to only inject if we
  // receive the onCommitted for the main_frame (frameId === 0).
  if (
    details.frameId === 0 &&
    (details.url.startsWith('https://') || details.url.startsWith('http://'))
  ) {
    chrome.tabs.executeScript(details.tabId, {
      file: 'bundles/web-discovery-content-script.bundle.js',
      matchAboutBlank: false,
      runAt: 'document_end'
    }, () => {
      if (chrome.runtime.lastError) {
        console.warn('[web-discovery-project] Could not inject script:', chrome.runtime.lastError)
      }
    })
  }
}

if (App !== undefined) {
  const APP = new App({
    version: chrome.runtime.getManifest().version
  })
  window.WDP = APP

  const WEB_DISCOVERY_PREF_KEY = 'brave.web_discovery_enabled'

  const toggleWebDiscovery = (pref?: chrome.settingsPrivate.PrefObject) => {
    if (pref && pref.type === chrome.settingsPrivate.PrefType.BOOLEAN) {
      const enable = pref.value
      if (enable) {
        // enable
        APP.start()
          .then(
            () => {
              // Dynamically inject WDP content script so that users with the pref disabled
              // don't need to pay the loading cost on each page.
              chrome.webNavigation.onCommitted.addListener(onCommitted)
            },
            (err) => { console.error('[web-discovery]', err) }
          )
      } else {
        // Stop injecting dynamic content scripts
        if (chrome.webNavigation.onCommitted.hasListener(onCommitted)) {
          chrome.webNavigation.onCommitted.removeListener(onCommitted)
        }

        // disable
        if (APP.isRunning) {
          APP.stop()
        }
      }
    }
  }

  chrome.settingsPrivate.onPrefsChanged.addListener((prefs: chrome.settingsPrivate.PrefObject[]) => {
    const pref = prefs.find(p => p.key === WEB_DISCOVERY_PREF_KEY)
    toggleWebDiscovery(pref)
  })

  chrome.settingsPrivate.getPref(WEB_DISCOVERY_PREF_KEY, (pref: chrome.settingsPrivate.PrefObject) => {
    toggleWebDiscovery(pref)
  })
}
