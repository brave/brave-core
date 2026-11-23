// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './settings_section_test-chromium.js'

import {
  OpenWindowProxyImpl,
  // <if expr="is_win or is_macosx">
  PasskeysBrowserProxyImpl,
  // </if>
  PasswordManagerImpl,
  PrefsBrowserProxy,
  PrefService,
  SyncBrowserProxyImpl,
} from 'chrome://password-manager/password_manager.js'
import type {
  PrefToggleButtonElement,
  SettingsSectionElement,
} from 'chrome://password-manager/password_manager.js'
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {flushTasks} from 'chrome://webui-test/polymer_test_util.js'
import {TestOpenWindowProxy} from 'chrome://webui-test/test_open_window_proxy.js'

// <if expr="is_win or is_macosx">
import {TestPasskeysBrowserProxy} from './test_passkeys_browser_proxy.js'
// </if>
import {TestPasswordManagerProxy} from './test_password_manager_proxy.js'
import {TestPrefsBrowserProxy} from './test_prefs_browser_proxy.js'
import {TestSyncBrowserProxy} from './test_sync_browser_proxy.js'
import {makePasswordManagerPrefs} from './test_util.js'

const FILL_ENABLED_PREF = 'brave.password_manager.fill_enabled'

// Brave adds its autofill toggle to the settings with a lit_mangler, which
// applies silently if upstream changes the template.
suite('BraveSettingsSection', () => {
  let prefsBrowserProxy: TestPrefsBrowserProxy
  let prefService: PrefService

  setup(async () => {
    document.body.replaceChildren()
    prefsBrowserProxy = new TestPrefsBrowserProxy([
      ...makePasswordManagerPrefs(),
      {
        key: FILL_ENABLED_PREF,
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: true,
      },
    ])
    PrefsBrowserProxy.setInstance(prefsBrowserProxy)
    PrefService.resetInstanceForTesting()
    prefService = PrefService.getInstance()
    await prefService.whenInitialized()
    PasswordManagerImpl.setInstance(new TestPasswordManagerProxy())
    OpenWindowProxyImpl.setInstance(new TestOpenWindowProxy())
    SyncBrowserProxyImpl.setInstance(new TestSyncBrowserProxy())
    // <if expr="is_win or is_macosx">
    PasskeysBrowserProxyImpl.setInstance(new TestPasskeysBrowserProxy())
    // </if>
  })

  async function createSettingsSection(): Promise<SettingsSectionElement> {
    const settings = document.createElement('settings-section')
    document.body.appendChild(settings)
    await flushTasks()
    return settings
  }

  function getFillToggle(
    settings: SettingsSectionElement,
  ): PrefToggleButtonElement {
    const toggle =
      settings.shadowRoot.querySelector<PrefToggleButtonElement>(
        '#braveFillToggle',
      )
    assertTrue(!!toggle)
    return toggle
  }

  test('fill toggle follows the save passwords toggle', async () => {
    const settings = await createSettingsSection()

    assertEquals(
      settings.$.passwordToggle,
      getFillToggle(settings).previousElementSibling,
    )
  })

  test('fill toggle follows the extension indicator', async () => {
    prefsBrowserProxy.fakeApi.sendPrefChanges([
      {
        key: 'credentials_enable_service',
        value: true,
        extensionId: 'test',
        controlledByName: 'test extension',
      },
    ])
    const settings = await createSettingsSection()

    const indicatorRow = settings.$.passwordToggle.nextElementSibling
    assertTrue(!!indicatorRow?.querySelector('#passwordsExtensionIndicator'))
    assertEquals(indicatorRow, getFillToggle(settings).previousElementSibling)
  })

  test('fill toggle shows the pref value', async () => {
    prefService.setPrefValue(FILL_ENABLED_PREF, false)
    const settings = await createSettingsSection()

    assertFalse(getFillToggle(settings).checked)
  })

  test('clicking the fill toggle updates the pref', async () => {
    const settings = await createSettingsSection()
    const toggle = getFillToggle(settings)
    assertTrue(toggle.checked)

    toggle.click()
    assertFalse(prefService.getPref<boolean>(FILL_ENABLED_PREF).value)
    assertFalse(toggle.checked)
  })
})
