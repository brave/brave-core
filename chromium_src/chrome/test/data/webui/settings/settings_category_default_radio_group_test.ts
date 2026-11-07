// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './settings_category_default_radio_group_test-chromium.js'

import type {SettingsCategoryDefaultRadioGroupElement} from 'chrome://settings/lazy_load.js'
import {
  ContentSetting,
  ContentSettingsTypes,
  SiteSettingsBrowserProxyImpl,
} from 'chrome://settings/lazy_load.js'
import {PrefService, PrefsBrowserProxy} from 'chrome://settings/settings.js'
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestPrefsBrowserProxy} from './test_prefs_browser_proxy.js'
import {TestSiteSettingsBrowserProxy} from './test_site_settings_browser_proxy.js'
import {
  createContentSettingTypeToValuePair,
  createDefaultContentSetting,
  createSiteSettingsPrefs,
} from './test_util.js'

// `PrefService` only creates the synthetic default pref for listed categories,
// so Brave's categories must be listed for their radios to show and save.
suite('BraveSettingsCategoryDefaultRadioGroup', () => {
  let proxy: TestSiteSettingsBrowserProxy
  let element: SettingsCategoryDefaultRadioGroupElement

  const categories = [
    ContentSettingsTypes.AUTOPLAY,
    ContentSettingsTypes.BRAVE_OPEN_AI_CHAT,
    ContentSettingsTypes.CARDANO,
    ContentSettingsTypes.ETHEREUM,
    ContentSettingsTypes.GOOGLE_SIGN_IN,
    ContentSettingsTypes.SOLANA,
  ]

  setup(async () => {
    proxy = new TestSiteSettingsBrowserProxy()
    SiteSettingsBrowserProxyImpl.setInstance(proxy)
    PrefsBrowserProxy.setInstance(new TestPrefsBrowserProxy([]))
    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()
    document.body.replaceChildren()
  })

  categories.forEach((category) => {
    test(`${category} shows its value and saves a click`, async () => {
      proxy.setPrefs(createSiteSettingsPrefs(
          [createContentSettingTypeToValuePair(
              category,
              createDefaultContentSetting({setting: ContentSetting.ALLOW}))],
          []))

      element = document.createElement('settings-category-default-radio-group')
      element.allowOptionLabel = 'Allow'
      element.blockOptionLabel = 'Block'
      element.category = category
      document.body.appendChild(element)
      await proxy.whenCalled('getDefaultValueForContentType')
      await microtasksFinished()

      assertTrue(element.$.allowRadioOption.checked)
      assertFalse(element.$.blockRadioOption.checked)

      proxy.resetResolver('setDefaultValueForContentType')
      element.$.blockRadioOption.click()
      const [savedCategory, savedSetting] =
          await proxy.whenCalled('setDefaultValueForContentType')
      assertEquals(category, savedCategory)
      assertEquals(ContentSetting.BLOCK, savedSetting)
      await microtasksFinished()
      assertTrue(element.$.blockRadioOption.checked)
    })
  })
})
