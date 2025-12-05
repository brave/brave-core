/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StateStore } from '$web-common/state_store'

import { AppState, AppActions, SiteBlockInfo, SiteSettings } from './app_state'

// Action handler decorator that adds optimistic local state updates for all
// mutation actions.
export function withOptimisticUpdates(
  store: StateStore<AppState>,
  actions: AppActions,
): AppActions {
  function updateSiteBlockInfo(info: Partial<SiteBlockInfo>) {
    store.update((state) => ({
      siteBlockInfo: {
        ...state.siteBlockInfo,
        ...info,
      },
    }))
  }

  function updateSiteSettings(settings: Partial<SiteSettings>) {
    store.update((state) => ({
      siteSettings: {
        ...state.siteSettings,
        ...settings,
      },
    }))
  }

  return {
    ...actions,

    setShieldsEnabled(enabled) {
      actions.setShieldsEnabled(enabled)
      updateSiteBlockInfo({ isBraveShieldsEnabled: enabled })
    },

    setAdBlockOnlyModeEnabled(enabled) {
      actions.setAdBlockOnlyModeEnabled(enabled)
      if (enabled) {
        updateSiteBlockInfo({ isBraveShieldsEnabled: true })
      }
      updateSiteBlockInfo({ isBraveShieldsAdBlockOnlyModeEnabled: enabled })
    },

    dismissAdBlockOnlyModePrompt() {
      actions.dismissAdBlockOnlyModePrompt()
      updateSiteBlockInfo({ showShieldsDisabledAdBlockOnlyModePrompt: false })
    },

    setShowAdvancedSettings(showAdvancedSettings) {
      actions.setShowAdvancedSettings(showAdvancedSettings)
      store.update({ showAdvancedSettings })
    },

    setAdBlockMode(mode) {
      actions.setAdBlockMode(mode)
      updateSiteSettings({ adBlockMode: mode })
    },

    setHttpsUpgradeMode(mode) {
      actions.setHttpsUpgradeMode(mode)
      updateSiteSettings({ httpsUpgradeMode: mode })
    },

    setIsNoScriptsEnabled(enabled) {
      actions.setIsNoScriptsEnabled(enabled)
      updateSiteSettings({ isNoscriptEnabled: enabled })
    },

    setFingerprintMode(mode) {
      actions.setFingerprintMode(mode)
      updateSiteSettings({ fingerprintMode: mode })
    },

    setCookieBlockMode(mode) {
      actions.setCookieBlockMode(mode)
      updateSiteSettings({ cookieBlockMode: mode })
    },

    setForgetFirstPartyStorageEnabled(enabled) {
      actions.setForgetFirstPartyStorageEnabled(enabled)
      updateSiteSettings({ isForgetFirstPartyStorageEnabled: enabled })
    },

    setWebcompatEnabled(feature, enabled) {
      actions.setWebcompatEnabled(feature, enabled)
      const { webcompatSettings } = store.getState().siteSettings
      updateSiteSettings({
        webcompatSettings: { ...webcompatSettings, [feature]: enabled },
      })
    },

    allowScriptsOnce(urls) {
      actions.allowScriptsOnce(urls)
      const { blockedJsList, allowedJsList } = store.getState().siteBlockInfo
      moveArrayItems(urls, blockedJsList, allowedJsList, (url, item) => {
        return item.url.startsWith(url)
      })
      updateSiteBlockInfo({
        blockedJsList: [...blockedJsList],
        allowedJsList: [...allowedJsList],
      })
    },

    blockAllowedScripts(urls) {
      actions.blockAllowedScripts(urls)
      const { blockedJsList, allowedJsList } = store.getState().siteBlockInfo
      moveArrayItems(urls, allowedJsList, blockedJsList, (url, item) => {
        return item.url.startsWith(url)
      })
      updateSiteBlockInfo({
        blockedJsList: [...blockedJsList],
        allowedJsList: [...allowedJsList],
      })
    },

    resetBlockedElements() {
      actions.resetBlockedElements()
      store.update({ blockedElementsPresent: false })
    },
  }
}

function moveArrayItems<T, U>(
  values: T[],
  from: U[],
  to: U[],
  filterPredicate: (value: T, item: U) => boolean,
) {
  for (const value of values) {
    const matches = from.filter((item) => filterPredicate(value, item))
    for (const match of matches) {
      const index = from.indexOf(match)
      if (index >= 0) {
        from.splice(index, 1)
        to.push(match)
      }
    }
  }
}
