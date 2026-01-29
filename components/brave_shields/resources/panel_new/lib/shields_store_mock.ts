/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  ContentSetting,
  ContentSettingSource,
  ContentSettingsType,
  defaultShieldsStore,
  withOptimisticUpdates,
} from './shields_store'

export function createShieldsStore() {
  const store = defaultShieldsStore()

  store.update((state) => ({
    initialized: true,
    siteBlockInfo: {
      ...state.siteBlockInfo,
      faviconUrl: { url: 'https://brave.com/favicon.ico' },
      isBraveShieldsEnabled: true,
      host: 'example.com',
      totalBlockedResources: 0,
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

    actions: withOptimisticUpdates(store),
  }))

  return store
}
