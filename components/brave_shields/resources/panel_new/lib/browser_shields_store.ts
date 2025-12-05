/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '$web-common/loadTimeData'
import { UIHandlerReceiver } from 'gen/brave/components/brave_shields/core/common/brave_shields_panel.mojom.m'
import { defaultShieldsStore, withOptimisticUpdates } from './shields_store'
import { ShieldsPanelProxy } from './shields_panel_proxy'

function loadFlag(key: string) {
  return loadTimeData.getBoolean(key)
}

function hasReloadsDetectedFlag() {
  const urlParams = new URLSearchParams(window.location.search)
  return urlParams.get('mode') === 'afterRepeatedReloads'
}

export function createShieldsStore() {
  const store = defaultShieldsStore()
  const { dataHandler, panelHandler } = ShieldsPanelProxy.getInstance()

  async function maybeReinitialize() {
    if (document.visibilityState === 'visible') {
      dataHandler.updateFavicon()
      await initialize()
    }
  }

  function addObservers() {
    dataHandler.registerUIHandler(
      new UIHandlerReceiver({
        onSiteBlockInfoChanged: (siteBlockInfo) => {
          const { host } = store.getState().siteBlockInfo
          const hostChanged = siteBlockInfo.host !== host
          if (hostChanged) {
            store.update({ initialized: false })
            maybeReinitialize()
          }
          store.update({ siteBlockInfo })
        },
      }).$.bindNewPipeAndPassRemote(),
    )

    document.addEventListener('visibilitychange', maybeReinitialize)
  }

  async function initialize() {
    store.update({
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
    })

    await Promise.all([
      panelHandler.getBrowserWindowHeight().then(({ height }) => {
        store.update({ browserWindowHeight: height })
      }),
      panelHandler.getAdvancedViewEnabled().then(({ isEnabled }) => {
        store.update({ showAdvancedSettings: isEnabled })
      }),
      dataHandler.getSiteSettings().then(store.update),
      dataHandler.getSiteBlockInfo().then(store.update),
      dataHandler.areAnyBlockedElementsPresent().then(({ isAvailable }) => {
        store.update({ blockedElementsPresent: isAvailable })
      }),
    ])

    store.update({ initialized: true })
  }

  initialize().then(addObservers)

  store.update({
    actions: withOptimisticUpdates(store, {
      notifyAppReady() {
        panelHandler.showUI()
      },

      closeUI() {
        panelHandler.closeUI()
      },

      setShowAdvancedSettings(showAdvancedSettings) {
        panelHandler.setAdvancedViewEnabled(showAdvancedSettings)
      },

      setShieldsEnabled(enabled) {
        dataHandler.setBraveShieldsEnabled(enabled)
      },

      setAdBlockOnlyModeEnabled(enabled) {
        dataHandler.setBraveShieldsAdBlockOnlyModeEnabled(enabled)
      },

      dismissAdBlockOnlyModePrompt() {
        dataHandler.setBraveShieldsAdBlockOnlyModePromptDismissed()
      },

      setAdBlockMode(mode) {
        dataHandler.setAdBlockMode(mode)
      },

      setHttpsUpgradeMode(mode) {
        dataHandler.setHttpsUpgradeMode(mode)
      },

      setIsNoScriptsEnabled(enabled) {
        dataHandler.setIsNoScriptsEnabled(enabled)
      },

      setFingerprintMode(mode) {
        dataHandler.setFingerprintMode(mode)
      },

      setCookieBlockMode(mode) {
        dataHandler.setCookieBlockMode(mode)
      },

      setForgetFirstPartyStorageEnabled(enabled) {
        dataHandler.setForgetFirstPartyStorageEnabled(enabled)
      },

      setWebcompatEnabled(feature, enabled) {
        dataHandler.setWebcompatEnabled(feature, enabled)
      },

      resetBlockedElements() {
        dataHandler.resetBlockedElements()
      },

      openWebCompatReporter() {
        dataHandler.openWebCompatWindow()
      },

      allowScriptsOnce(urls) {
        dataHandler.allowScriptsOnce(urls)
      },

      blockAllowedScripts(urls) {
        dataHandler.blockAllowedScripts(urls)
      },

      openTab(url: string) {
        chrome.tabs.create({ url, active: true })
      },
    }),
  })

  return store
}
