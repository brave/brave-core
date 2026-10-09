// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './privacy_page_test-chromium.js'

import 'chrome://settings/lazy_load.js'

import {ClearBrowsingDataBrowserProxyImpl} from 'chrome://settings/lazy_load.js'
import type {SettingsPrivacyPageElement} from 'chrome://settings/settings.js'
import {
  loadTimeData,
  MetricsBrowserProxyImpl,
  PrefService,
  PrefsBrowserProxy,
  resetRouterForTesting,
} from 'chrome://settings/settings.js'
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {isChildVisible, microtasksFinished} from 'chrome://webui-test/test_util.js'

import {getInitialPrivacyGuideTestPrefs} from './privacy_guide_test_util.js'
import {TestClearBrowsingDataBrowserProxy} from './test_clear_browsing_data_browser_proxy.js'
import {TestMetricsBrowserProxy} from './test_metrics_browser_proxy.js'
import {TestPrefsBrowserProxy} from './test_prefs_browser_proxy.js'

function pref(
    key: string, type: chrome.settingsPrivate.PrefType,
    value: boolean|number|string): chrome.settingsPrivate.PrefObject {
  return {key, type, value}
}

// The prefs Brave's personalization options bind their toggles to.
function getBravePrefs(): chrome.settingsPrivate.PrefObject[] {
  const {BOOLEAN, NUMBER, STRING} = chrome.settingsPrivate.PrefType
  return [
    pref('brave.history_embeddings_enabled', BOOLEAN, true),
    pref('brave.history.retention_days', NUMBER, 0),
    pref('webrtc.ip_handling_policy', STRING, 'default'),
    pref('brave.gcm.channel_status', BOOLEAN, true),
    pref('brave.de_amp.enabled', BOOLEAN, true),
    pref('brave.debounce.enabled', BOOLEAN, true),
    pref('brave.reduce_language', BOOLEAN, true),
    pref('brave.request_otr.request_otr_action_option', NUMBER, 0),
    pref('brave.psst.settings.enable_psst', BOOLEAN, true),
    pref('enable_do_not_track', BOOLEAN, false),
  ]
}

// Brave adds its own personalization options to the privacy page and hides the
// rows it replaces, with a lit_mangler. Check the result, since it applies
// silently if upstream changes the template.
suite('BravePrivacyPage', () => {
  let page: SettingsPrivacyPageElement

  async function createPage() {
    document.body.innerHTML = window.trustedTypes!.emptyHTML
    page = document.createElement('settings-privacy-page')
    document.body.appendChild(page)
    await microtasksFinished()
  }

  setup(async () => {
    loadTimeData.overrideValues({showPrivacyGuide: true})
    resetRouterForTesting()

    PrefsBrowserProxy.setInstance(new TestPrefsBrowserProxy([
      ...getInitialPrivacyGuideTestPrefs(),
      ...getBravePrefs(),
    ]))
    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()

    ClearBrowsingDataBrowserProxyImpl.setInstance(
        new TestClearBrowsingDataBrowserProxy())
    MetricsBrowserProxyImpl.setInstance(new TestMetricsBrowserProxy())
    await createPage()
  })

  teardown(() => {
    page.remove()
    resetRouterForTesting()
  })

  test('the personalization options element is defined', () => {
    // It is registered by a separate import, which the mangler can't add.
    assertTrue(!!customElements.get('settings-brave-personalization-options'))
  })

  test('the personalization options follow the site settings row', () => {
    const siteSettings =
        page.shadowRoot.querySelector<HTMLElement>('#siteSettingsLinkRow')
    assertTrue(!!siteSettings)
    const options = siteSettings.nextElementSibling
    assertTrue(!!options)
    assertEquals(
        'settings-brave-personalization-options',
        options.tagName.toLowerCase())
  })

  test('the personalization options render', () => {
    const options = page.shadowRoot.querySelector(
        'settings-brave-personalization-options')
    assertTrue(!!options)
    assertTrue(!!options.shadowRoot)
    assertTrue(!!options.shadowRoot.querySelector('#pushMessagingEnabled'))
    assertTrue(!!options.shadowRoot.querySelector('#doNotTrack'))
  })

  test('the third party cookies row is hidden', () => {
    assertTrue(
        !!page.shadowRoot.querySelector('#thirdPartyCookiesLinkRow'))
    assertFalse(isChildVisible(page, '#thirdPartyCookiesLinkRow'))
  })

  test('the privacy guide row is hidden even when it is available', () => {
    // Upstream shows it when `showPrivacyGuide` is set.
    const row =
        page.shadowRoot.querySelector<HTMLElement>('#privacyGuideLinkRow')
    assertTrue(!!row)
    assertTrue(row.hidden)
    assertFalse(isChildVisible(page, '#privacyGuideLinkRow'))
  })
})
