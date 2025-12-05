/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateStore } from '../lib/state_store'

import {
  AppActions,
  defaultAppState,
  defaultAppActions,
  ContentSetting,
  ContentSettingSource,
  ContentSettingsType,
  SiteBlockInfo,
  SiteSettings,
} from './app_state'

export function createAppState() {
  const store = createStateStore(defaultAppState())

  store.update((state) => ({
    initialized: true,
    siteBlockInfo: {
      ...state.siteBlockInfo,
      isBraveShieldsEnabled: true,
      host: 'example.com',
      totalBlockedResources: 42,
      adsList: [
        { url: 'https://example.com/1' },
        { url: 'https://example.com/2?abc' },
        { url: 'https://example2.com/1' },
      ],
      allowedJsList: [{ url: 'https://example.com/1' }],
      blockedJsList: [
        { url: 'https://example.com/1' },
        { url: 'https://example.com/2' },
        { url: 'https://example2.com/1' },
      ],
      invokedWebcompatList: [
        ContentSettingsType.BRAVE_WEBCOMPAT_AUDIO,
        ContentSettingsType.BRAVE_WEBCOMPAT_CANVAS,
        ContentSettingsType.BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY,
      ],
      isBraveShieldsAdBlockOnlyModeEnabled: false,
      showShieldsDisabledAdBlockOnlyModePrompt: true,
    },
    siteSettings: {
      ...state.siteSettings,
      scriptsBlockedOverrideStatus: {
        status: ContentSetting.BLOCK,
        overrideSource: ContentSettingSource.kNone,
      },
      webcompatSettings: {},
    },
    isHttpsByDefaultEnabled: true,
    blockedElementsPresent: true,
    isWebcompatExceptionsServiceEnabled: true,
  }))

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

  const actions = <AppActions>{
    ...defaultAppActions(),

    setShieldsEnabled(enabled) {
      updateSiteBlockInfo({ isBraveShieldsEnabled: enabled })
    },

    setAdBlockOnlyModeEnabled(enabled) {
      if (enabled) {
        updateSiteBlockInfo({ isBraveShieldsEnabled: true })
      }
      updateSiteBlockInfo({ isBraveShieldsAdBlockOnlyModeEnabled: enabled })
    },

    dismissAdBlockOnlyModePrompt() {
      updateSiteBlockInfo({ showShieldsDisabledAdBlockOnlyModePrompt: false })
    },

    setShowAdvancedSettings(showAdvancedSettings) {
      store.update({ showAdvancedSettings })
    },

    setAdBlockMode(mode) {
      updateSiteSettings({ adBlockMode: mode })
    },

    setHttpsUpgradeMode(mode) {
      updateSiteSettings({ httpsUpgradeMode: mode })
    },

    setIsNoScriptsEnabled(enabled) {
      updateSiteSettings({ isNoscriptEnabled: enabled })
    },

    setFingerprintMode(mode) {
      updateSiteSettings({ fingerprintMode: mode })
    },

    setCookieBlockMode(mode) {
      updateSiteSettings({ cookieBlockMode: mode })
    },

    setForgetFirstPartyStorageEnabled(enabled) {
      updateSiteSettings({ isForgetFirstPartyStorageEnabled: enabled })
    },

    setWebcompatEnabled(feature, enabled) {
      const { webcompatSettings } = store.getState().siteSettings
      updateSiteSettings({
        webcompatSettings: { ...webcompatSettings, [feature]: enabled },
      })
    },

    allowScriptsOnce(urls) {
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
      store.update({ blockedElementsPresent: false })
    },
  }

  return { store, actions }
}
