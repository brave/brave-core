// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './people_page_sync_controls_test-chromium.js'

import 'chrome://settings/lazy_load.js'

import {webUIListenerCallback} from 'chrome://resources/js/cr.js'
import type {SettingsSyncControlsElement} from 'chrome://settings/lazy_load.js'
import type {CrToggleElement} from 'chrome://settings/settings.js'
import {
  loadTimeData,
  SignedInState,
  StatusAction,
  SyncBrowserProxyImpl,
  UserSelectableType,
} from 'chrome://settings/settings.js'
import {
  assertEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {isVisible, microtasksFinished} from 'chrome://webui-test/test_util.js'

import {getSyncAllPrefs} from './sync_test_util.js'
import {TestSyncBrowserProxy} from './test_sync_browser_proxy.js'

// Brave removes the payments toggle and adds an AI Chat one with a
// lit_mangler. Check the result, since it applies silently if upstream changes
// the template.
suite('BraveSyncControls', () => {
  // The AI Chat label is only registered in builds with AI Chat.
  const hasAiChat = loadTimeData.valueExists('aiChatCheckboxLabel')
  const aiChatTest = hasAiChat ? test : test.skip

  let syncControls: SettingsSyncControlsElement
  let browserProxy: TestSyncBrowserProxy

  async function setPrefs(
      overrides: Partial<ReturnType<typeof getSyncAllPrefs>> = {}) {
    // Individual toggles are only enabled when not syncing everything.
    const prefs = {
      ...getSyncAllPrefs(),
      syncAllDataTypes: false,
      ...overrides,
    }
    webUIListenerCallback('sync-prefs-changed', prefs)
    await microtasksFinished()
  }

  // Toggles write either all the sync prefs or a single data type, depending on
  // the page.
  async function setAccountSettingsPage(isAccountSettingsPage: boolean) {
    (syncControls as unknown as {isAccountSettingsPage_: boolean})
        .isAccountSettingsPage_ = isAccountSettingsPage
    await microtasksFinished()
  }

  // Brave's `cr-toggle` wraps a Nala toggle, which doesn't react to clicks on
  // the host. Report a user change the way the Nala toggle does.
  async function flipToggle(toggle: CrToggleElement) {
    await (toggle as unknown as {
             onChange_(e: {checked: boolean}): Promise<void>,
           }).onChange_({checked: !toggle.checked})
  }

  function getAiChatToggle(): CrToggleElement {
    const toggle = syncControls.shadowRoot.querySelector<CrToggleElement>(
        'cr-toggle[data-pref="aiChatSynced"]')
    assertTrue(!!toggle)
    return toggle
  }

  setup(async () => {
    // The AI Chat toggle is only rendered when sync of it is enabled.
    if (hasAiChat) {
      loadTimeData.overrideValues({isBraveSyncAIChatEnabled: true})
    }

    browserProxy = new TestSyncBrowserProxy()
    SyncBrowserProxyImpl.setInstance(browserProxy)

    document.body.innerHTML = window.trustedTypes!.emptyHTML
    syncControls = document.createElement('settings-sync-controls')
    syncControls.syncStatus = {
      signedInState: SignedInState.SYNCING,
      statusAction: StatusAction.NO_ACTION,
    }
    document.body.appendChild(syncControls)
    await setPrefs()
  })

  test('the payments toggle is removed', () => {
    assertEquals(
        null, syncControls.shadowRoot.querySelector('#paymentsCheckbox'))
  })

  aiChatTest('the AI Chat toggle is shown with its label', () => {
    const row = getAiChatToggle().parentElement!
    assertTrue(isVisible(row))
    assertEquals(
        loadTimeData.getString('aiChatCheckboxLabel'),
        row.querySelector('#aiChatCheckboxLabel')!.textContent.trim())
  })

  aiChatTest('the AI Chat toggle reflects its pref', async () => {
    assertTrue(getAiChatToggle().checked)
    await setPrefs({aiChatSynced: false})
    assertFalse(getAiChatToggle().checked)
  })

  aiChatTest('the AI Chat toggle is hidden when unregistered', async () => {
    await setPrefs({aiChatRegistered: false})
    assertFalse(isVisible(getAiChatToggle().parentElement))
  })

  aiChatTest('the AI Chat toggle is disabled when it is managed', async () => {
    assertFalse(getAiChatToggle().disabled)
    await setPrefs({aiChatManaged: true})
    assertTrue(getAiChatToggle().disabled)
    assertTrue(isVisible(
        getAiChatToggle().parentElement!.querySelector('cr-policy-indicator')))
  })

  aiChatTest('changing the AI Chat toggle writes the sync prefs', async () => {
    await setAccountSettingsPage(false)
    await flipToggle(getAiChatToggle())
    const prefs = await browserProxy.whenCalled('setSyncDatatypes')
    assertFalse(prefs.aiChatSynced)
  })

  aiChatTest('the account page writes the AI Chat data type', async () => {
    await setAccountSettingsPage(true)
    await flipToggle(getAiChatToggle())
    const [type, value] = await browserProxy.whenCalled('setSyncDatatype')
    assertEquals(UserSelectableType.AI_CHAT, type)
    assertFalse(value)
  })
})
