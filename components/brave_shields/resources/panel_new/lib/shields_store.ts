/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  SiteBlockInfo,
  SiteSettings,
} from 'gen/brave/components/brave_shields/core/common/brave_shields_panel.mojom.m.js'

import {
  AdBlockMode,
  HttpsUpgradeMode,
  FingerprintMode,
  CookieBlockMode,
  ContentSettingSource,
} from 'gen/brave/components/brave_shields/core/common/shields_settings.mojom.m.js'

export { ContentSetting } from 'gen/components/content_settings/core/common/content_settings.mojom.m'

import { ContentSettingsType } from 'gen/components/content_settings/core/common/content_settings_types.mojom.m'

import { StateStore, createStateStore } from '$web-common/state_store'

export {
  SiteBlockInfo,
  SiteSettings,
  AdBlockMode,
  HttpsUpgradeMode,
  FingerprintMode,
  CookieBlockMode,
  ContentSettingSource,
  ContentSettingsType,
}

export interface ShieldsActions {
  notifyAppReady: () => void
  closeUI: () => void
  setShowAdvancedSettings: (showAdvancedSettings: boolean) => void
  setShieldsEnabled: (enabled: boolean) => void
  setAdBlockOnlyModeEnabled: (enabled: boolean) => void
  dismissAdBlockOnlyModePrompt: () => void
  setAdBlockMode: (mode: AdBlockMode) => void
  setHttpsUpgradeMode: (mode: HttpsUpgradeMode) => void
  setIsNoScriptsEnabled: (enabled: boolean) => void
  setFingerprintMode: (mode: FingerprintMode) => void
  setCookieBlockMode: (mode: CookieBlockMode) => void
  setForgetFirstPartyStorageEnabled: (enabled: boolean) => void
  setWebcompatEnabled: (feature: ContentSettingsType, enabled: boolean) => void
  resetBlockedElements: () => void
  openWebCompatReporter: () => void
  allowScriptsOnce: (urls: string[]) => void
  blockAllowedScripts: (urls: string[]) => void
  openTab: (url: string) => void
}

export interface ShieldsState {
  initialized: boolean
  browserWindowHeight: number
  showAdvancedSettings: boolean
  siteSettings: SiteSettings
  siteBlockInfo: SiteBlockInfo
  blockedElementsPresent: boolean
  isHttpsByDefaultEnabled: boolean
  isTorProfile: boolean
  showStrictFingerprintingMode: boolean
  isWebcompatExceptionsServiceEnabled: boolean
  isBraveForgetFirstPartyStorageFeatureEnabled: boolean
  repeatedReloadsDetected: boolean
  actions: ShieldsActions
}

export type ShieldsStore = StateStore<ShieldsState>

export function defaultShieldsStore() {
  return createStateStore<ShieldsState>({
    initialized: false,
    browserWindowHeight: 0,
    showAdvancedSettings: false,
    siteSettings: {
      adBlockMode: AdBlockMode.STANDARD,
      httpsUpgradeMode: HttpsUpgradeMode.STANDARD_MODE,
      fingerprintMode: FingerprintMode.STANDARD_MODE,
      cookieBlockMode: CookieBlockMode.BLOCKED,
      isNoscriptEnabled: false,
      isForgetFirstPartyStorageEnabled: false,
      scriptsBlockedOverrideStatus: undefined,
      webcompatSettings: {},
    },
    siteBlockInfo: {
      host: '',
      totalBlockedResources: 0,
      isBraveShieldsAdBlockOnlyModeEnabled: false,
      showShieldsDisabledAdBlockOnlyModePrompt: false,
      isBraveShieldsEnabled: false,
      isBraveShieldsManaged: false,
      faviconUrl: { url: '' },
      adsList: [],
      httpRedirectsList: [],
      blockedJsList: [],
      allowedJsList: [],
      fingerprintsList: [],
      invokedWebcompatList: [],
    },
    blockedElementsPresent: false,
    isHttpsByDefaultEnabled: false,
    isTorProfile: false,
    showStrictFingerprintingMode: false,
    isWebcompatExceptionsServiceEnabled: false,
    isBraveForgetFirstPartyStorageFeatureEnabled: false,
    repeatedReloadsDetected: false,

    actions: {
      notifyAppReady() {},
      closeUI() {},
      setShowAdvancedSettings(showAdvancedSettings) {},
      setShieldsEnabled(enabled) {},
      setAdBlockOnlyModeEnabled(enabled) {},
      dismissAdBlockOnlyModePrompt() {},
      setAdBlockMode(mode) {},
      setHttpsUpgradeMode(mode) {},
      setIsNoScriptsEnabled(enabled) {},
      setFingerprintMode(mode) {},
      setCookieBlockMode(mode) {},
      setForgetFirstPartyStorageEnabled(enabled) {},
      setWebcompatEnabled(feature, enabled) {},
      resetBlockedElements() {},
      openWebCompatReporter() {},
      allowScriptsOnce(urls) {},
      blockAllowedScripts(urls) {},
      openTab(url: string) {},
    },
  })
}

// Action decorator that adds optimistic local state updates for all mutations.
export function withOptimisticUpdates(
  store: ShieldsStore,
  actions?: ShieldsActions,
): ShieldsActions {
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

  actions = actions ?? store.getState().actions

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
