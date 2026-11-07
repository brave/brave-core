// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './settings_toggle_button_test-chromium.js'

import type {SettingsToggleButtonElement} from 'chrome://settings/settings.js'
import {PrefService, PrefsBrowserProxy} from 'chrome://settings/settings.js'
import {
  assertEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestPrefsBrowserProxy} from './test_prefs_browser_proxy.js'

// Upstream removed the top-level `prefs` object, so Brave pages bind their
// toggles by `pref-key`. Check that clicking one writes through to the
// backend.
suite('BraveSettingsToggleButtonPrefKey', () => {
  let prefsBrowserProxy: TestPrefsBrowserProxy
  let page: HTMLElement

  const initialPrefs = [
    {
      key: 'brave.google_login_default',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: true,
    },
    {
      key: 'brave.shields.fb_embed_default',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: true,
    },
    {
      key: 'brave.shields.twitter_embed_default',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: true,
    },
    {
      key: 'brave.shields.linkedin_embed_default',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: false,
    },
  ]

  setup(async () => {
    prefsBrowserProxy = new TestPrefsBrowserProxy(initialPrefs)
    PrefsBrowserProxy.setInstance(prefsBrowserProxy)

    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()

    document.body.replaceChildren()
    page = document.createElement('settings-social-blocking-page')
    document.body.appendChild(page)
    await microtasksFinished()
  })

  function getToggle(id: string): SettingsToggleButtonElement {
    const toggle =
        page.shadowRoot!.querySelector<SettingsToggleButtonElement>(`#${id}`)
    assertTrue(!!toggle)
    return toggle
  }

  test('toggle reflects its pref', () => {
    assertTrue(getToggle('fbEmbedControlType').checked)
    assertTrue(getToggle('twitterEmbedControlType').checked)
    assertFalse(getToggle('linkedInEmbedControlType').checked)
  })

  test('clicking a toggle writes its pref', async () => {
    const toggle = getToggle('fbEmbedControlType')
    assertTrue(toggle.checked)

    // Like upstream's pref-key suite, click the host: it toggles synchronously.
    toggle.click()
    assertFalse(toggle.checked)
    assertFalse(PrefService.getInstance()
                    .getPref<boolean>('brave.shields.fb_embed_default').value)
    const pref =
        await prefsBrowserProxy.getPref('brave.shields.fb_embed_default')
    assertEquals(false, pref.value)
  })

  test('pref changes update the toggle', async () => {
    const toggle = getToggle('linkedInEmbedControlType')
    assertFalse(toggle.checked)

    PrefService.getInstance().setPrefValue(
        'brave.shields.linkedin_embed_default', true)
    await microtasksFinished()
    assertTrue(toggle.checked)
  })
})
