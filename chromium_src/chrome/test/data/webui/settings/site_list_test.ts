// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './site_list_test-chromium.js'

import type {SiteListElement} from 'chrome://settings/lazy_load.js'
import {
  ContentSetting,
  ContentSettingsTypes,
  SiteSettingsBrowserProxyImpl,
} from 'chrome://settings/lazy_load.js'
import {loadTimeData} from 'chrome://settings/settings.js'
import {assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestSiteSettingsBrowserProxy} from './test_site_settings_browser_proxy.js'

// The Shields exceptions list labels its actions "Shields up" and "Shields
// down"; other lists keep "Allow" and "Block".
suite('BraveSiteListActionLabels', () => {
  let testElement: SiteListElement

  setup(() => {
    SiteSettingsBrowserProxyImpl.setInstance(new TestSiteSettingsBrowserProxy())
    document.body.replaceChildren()
    testElement = document.createElement('site-list')
    testElement.searchFilter = ''
    document.body.appendChild(testElement)
  })

  function getLabel(id: string): string {
    const button = testElement.shadowRoot.querySelector<HTMLElement>(`#${id}`)
    assertTrue(!!button)
    return button.textContent.trim()
  }

  test('Shields exceptions use the Shields labels', async () => {
    testElement.categorySubtype = ContentSetting.ALLOW
    testElement.category = ContentSettingsTypes.BRAVE_SHIELDS
    await microtasksFinished()

    assertEquals(loadTimeData.getString('siteSettingsShieldsUp'),
        getLabel('allow'))
    assertEquals(loadTimeData.getString('siteSettingsShieldsDown'),
        getLabel('block'))
  })

  test('other exceptions keep the default labels', async () => {
    testElement.categorySubtype = ContentSetting.ALLOW
    testElement.category = ContentSettingsTypes.COOKIES
    await microtasksFinished()

    assertEquals(loadTimeData.getString('siteSettingsActionAllow'),
        getLabel('allow'))
    assertEquals(loadTimeData.getString('siteSettingsActionBlock'),
        getLabel('block'))
  })
})
