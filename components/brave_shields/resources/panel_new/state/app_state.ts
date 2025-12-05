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

export interface AppState {
  initialized: boolean
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
}

export function defaultAppState(): AppState {
  return {
    initialized: false,
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
  }
}

export interface AppActions {
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

export function defaultAppActions(): AppActions {
  return {
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
  }
}
