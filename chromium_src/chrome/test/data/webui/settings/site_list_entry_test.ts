// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './site_list_entry_test-chromium.js'

import type {SiteListEntryElement} from 'chrome://settings/lazy_load.js'
import {
  ContentSettingsTypes,
  SiteSettingsBrowserProxyImpl,
} from 'chrome://settings/lazy_load.js'
import {loadTimeData} from 'chrome://settings/settings.js'
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {eventToPromise, microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestSiteSettingsBrowserProxy} from './test_site_settings_browser_proxy.js'
import {createSiteException} from './test_util.js'

// Exceptions that Shields or Google sign-in control can't be reset from the
// list: the reset button is disabled, and explains why on hover.
suite('BraveSiteListEntryReset', () => {
  let testElement: SiteListEntryElement

  setup(() => {
    SiteSettingsBrowserProxyImpl.setInstance(new TestSiteSettingsBrowserProxy())
    document.body.replaceChildren()
    testElement = document.createElement('site-list-entry')
    document.body.appendChild(testElement)
  })

  function getResetSite(): HTMLElement {
    const resetSite =
        testElement.shadowRoot.querySelector<HTMLElement>('#resetSite')
    assertTrue(!!resetSite)
    return resetSite
  }

  test('reset is enabled for a user exception', async () => {
    testElement.model = createSiteException(
        'https://example.com', {category: ContentSettingsTypes.COOKIES})
    await microtasksFinished()

    const resetSite = getResetSite()
    assertFalse(resetSite.hasAttribute('disabled'))
    assertEquals('trash', resetSite.getAttribute('iron-icon'))
  })

  test('reset is disabled with a tooltip under Shields', async () => {
    testElement.model = createSiteException('https://example.com', {
      category: ContentSettingsTypes.COOKIES,
      braveCookieType: 'shields settings',
    })
    await microtasksFinished()

    const resetSite = getResetSite()
    assertTrue(resetSite.hasAttribute('disabled'))

    const tooltip = eventToPromise('show-tooltip', testElement)
    resetSite.dispatchEvent(new MouseEvent('mouseenter'))
    const event = await tooltip as CustomEvent<{target: HTMLElement, text: string}>
    assertEquals(resetSite, event.detail.target)
    assertEquals(
        loadTimeData.getString('cookieControlledByShieldsTooltip'),
        event.detail.text)
  })

  test('reset is disabled with a tooltip under Google sign-in', async () => {
    testElement.model = createSiteException('https://example.com', {
      category: ContentSettingsTypes.COOKIES,
      braveCookieType: 'google sign-in',
    })
    await microtasksFinished()

    const resetSite = getResetSite()
    assertTrue(resetSite.hasAttribute('disabled'))

    const tooltip = eventToPromise('show-tooltip', testElement)
    resetSite.dispatchEvent(new MouseEvent('mouseenter'))
    const event = await tooltip as CustomEvent<{target: HTMLElement, text: string}>
    assertEquals(
        loadTimeData.getString('cookieControlledByGoogleSigninTooltip'),
        event.detail.text)
  })
})
