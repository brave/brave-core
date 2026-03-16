/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { makeCloseable, Closable } from '$web-common/api'
import { createShieldsApi } from './shields_api'

import {
  SiteBlockInfo,
  SiteSettings,
  DataHandlerInterface,
  PanelHandlerInterface,
} from 'gen/brave/components/brave_shields/core/common/brave_shields_panel.mojom.m.js'

import {
  AdBlockMode,
  FingerprintMode,
  CookieBlockMode,
  HttpsUpgradeMode,
  ContentSettingSource,
} from 'gen/brave/components/brave_shields/core/common/shields_settings.mojom.m.js'

import { ContentSetting } from 'gen/components/content_settings/core/common/content_settings.mojom.m'
import { ContentSettingsType } from 'gen/components/content_settings/core/common/content_settings_types.mojom.m'

const mockSiteBlockInfo: SiteBlockInfo = {
  host: 'example.com',
  totalBlockedResources: 5,
  isBraveShieldsAdBlockOnlyModeEnabled: false,
  showShieldsDisabledAdBlockOnlyModePrompt: true,
  isBraveShieldsEnabled: true,
  isBraveShieldsManaged: false,
  faviconUrl: { url: 'https://brave.com/favicon.ico' },
  adsList: [
    { url: 'https://example.com/ad1' },
    { url: 'https://example.com/ad2' },
    { url: 'https://tracker.com/pixel' },
  ],
  httpRedirectsList: [],
  blockedJsList: [
    { url: 'https://example.com/script1.js' },
    { url: 'https://example.com/script2.js' },
    { url: 'https://cdn.example.com/lib.js' },
  ],
  allowedJsList: [{ url: 'https://example.com/allowed.js' }],
  fingerprintsList: [{ url: 'https://example.com/fingerprint' }],
  invokedWebcompatList: [
    ContentSettingsType.BRAVE_WEBCOMPAT_AUDIO,
    ContentSettingsType.BRAVE_WEBCOMPAT_CANVAS,
    ContentSettingsType.BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY,
  ],
}

const mockSiteSettings: SiteSettings = {
  adBlockMode: AdBlockMode.STANDARD,
  fingerprintMode: FingerprintMode.STANDARD_MODE,
  cookieBlockMode: CookieBlockMode.BLOCKED,
  httpsUpgradeMode: HttpsUpgradeMode.STANDARD_MODE,
  isNoscriptEnabled: false,
  isForgetFirstPartyStorageEnabled: false,
  scriptsBlockedOverrideStatus: {
    status: ContentSetting.BLOCK,
    overrideSource: ContentSettingSource.kNone,
  },
  webcompatSettings: {},
}

function createMockDataHandler(): Closable<DataHandlerInterface> {
  return makeCloseable({
    registerUIHandler(handler) {},

    getSiteBlockInfo() {
      return Promise.resolve({ siteBlockInfo: mockSiteBlockInfo })
    },

    getSiteSettings() {
      return Promise.resolve({ siteSettings: mockSiteSettings })
    },

    areAnyBlockedElementsPresent() {
      return Promise.resolve({ isAvailable: true })
    },

    setBraveShieldsEnabled(isEnabled) {},
    setAdBlockMode(mode) {},
    setFingerprintMode(mode) {},
    setCookieBlockMode(mode) {},
    setHttpsUpgradeMode(mode) {},
    setIsNoScriptsEnabled(isEnabled) {},
    setBraveShieldsAdBlockOnlyModeEnabled(isEnabled) {},
    setBraveShieldsAdBlockOnlyModePromptDismissed() {},
    setForgetFirstPartyStorageEnabled(isEnabled) {},
    setWebcompatEnabled(webcompatSettingsType, enable) {},
    allowScriptsOnce(origins) {},
    blockAllowedScripts(origins) {},
    resetBlockedElements() {},
    openWebCompatWindow() {},
    updateFavicon() {},
  })
}

function createMockPanelHandler(): Closable<PanelHandlerInterface> {
  let advancedViewEnabled = false

  return makeCloseable({
    showUI() {},
    closeUI() {},

    getPosition() {
      return Promise.resolve({ vec: { x: 0, y: 0 } })
    },

    getBrowserWindowHeight() {
      return Promise.resolve({ height: 800 })
    },

    getAdvancedViewEnabled() {
      return Promise.resolve({ isEnabled: advancedViewEnabled })
    },

    setAdvancedViewEnabled(isEnabled) {
      advancedViewEnabled = isEnabled
    },
  })
}

export function createMockShieldsAPI() {
  return createShieldsApi({
    dataHandler: createMockDataHandler(),
    panelHandler: createMockPanelHandler(),
    openTab(url) {
      window.open(url, '_blank', 'noopener noreferrer')
    },
    loadTimeState: {
      isHttpsByDefaultEnabled: true,
      isTorProfile: false,
      showStrictFingerprintingMode: true,
      isWebcompatExceptionsServiceEnabled: true,
      isBraveForgetFirstPartyStorageFeatureEnabled: true,
      repeatedReloadsDetected: false,
    },
  })
}
