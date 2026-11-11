// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './clear_browsing_data_dialog_test-chromium.js'

import {PrefService, PrefsBrowserProxy} from 'chrome://settings/settings.js'
import {assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestPrefsBrowserProxy} from './test_prefs_browser_proxy.js'

// The `*_on_exit` prefs that Brave registers in
// chromium_src/components/browsing_data/core/pref_names.h.
const ON_EXIT_PREF_KEYS = [
  'browser.clear_data.browsing_history_on_exit',
  'browser.clear_data.download_history_on_exit',
  'browser.clear_data.cache_on_exit',
  'browser.clear_data.cookies_on_exit',
  'browser.clear_data.passwords_on_exit',
  'browser.clear_data.form_data_on_exit',
  'browser.clear_data.hosted_apps_data_on_exit',
  'browser.clear_data.site_settings_on_exit',
  'browser.clear_data.brave_leo_on_exit',
]

interface OnExitPage extends HTMLElement {
  getChangedSettings(): Array<{key: string, value: boolean}>
}

// The page lists upstream's data types, so a new one gets a checkbox bound to
// an `_on_exit` pref that doesn't exist, which throws when the page reads it.
// Check that every rendered checkbox has a registered pref.
suite('BraveClearBrowsingDataOnExitPage', () => {
  let page: OnExitPage

  setup(async () => {
    document.body.replaceChildren()

    PrefsBrowserProxy.setInstance(new TestPrefsBrowserProxy(
        ON_EXIT_PREF_KEYS.map((key) => ({
                                key,
                                type: chrome.settingsPrivate.PrefType.BOOLEAN,
                                value: false,
                              }))))
    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()

    page = document.createElement(
              'settings-brave-clear-browsing-data-on-exit-page') as OnExitPage
    document.body.appendChild(page)
    await microtasksFinished()
  })

  test('every checkbox is bound to a registered pref', () => {
    const checkboxes = page.shadowRoot!.querySelectorAll('settings-checkbox')
    assertTrue(checkboxes.length > 0)
    for (const checkbox of checkboxes) {
      assertTrue(
          ON_EXIT_PREF_KEYS.includes(checkbox.prefKey),
          `No on-exit pref for ${checkbox.prefKey}`)
    }
  })

  test('there is no Skills checkbox', () => {
    const keys = Array.from(page.shadowRoot!.querySelectorAll(
                                'settings-checkbox'))
                     .map((checkbox) => checkbox.prefKey)
    assertTrue(!keys.includes('browser.clear_data.skills_on_exit'))
  })

  test('reading the changed settings does not throw', () => {
    assertEquals(0, page.getChangedSettings().length)
  })
})
