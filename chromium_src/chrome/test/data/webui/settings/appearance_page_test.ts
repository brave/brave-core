// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './appearance_page_test-chromium.js'

import {sendWithPromise} from 'chrome://resources/js/cr.js'
import type {SettingsToggleButtonElement} from 'chrome://settings/settings.js'
import {PrefService} from 'chrome://settings/settings.js'
import {assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {eventToPromise, microtasksFinished} from 'chrome://webui-test/test_util.js'

// Brave removes upstream's Ctrl+Tab MRU toggle with a lit_mangler, since its own
// Content-page toggle drives the same browser.ctrl_tab_mru pref. Check the
// result, as the override applies silently if upstream moves the row.
suite('BraveAppearancePage', () => {
  test('upstream ctrl+tab MRU toggle is removed', async () => {
    const page = document.createElement('settings-appearance-page')
    document.body.appendChild(page)
    await microtasksFinished()

    // Brave's own section, so an unrendered page can't pass the check below.
    assertTrue(
      !!page.shadowRoot.querySelector('settings-brave-appearance-toolbar'))
    assertFalse(!!page.shadowRoot.querySelector('#ctrlTabMru'))
  })
})

// On macOS the tab strip can't be switched while the window is fullscreen, so
// the "Use vertical tabs" toggle asks the browser to switch once fullscreen is
// exited. BraveAppearanceTabsFullscreenWebUITest enters fullscreen before this
// suite runs and checks the toast and the switch afterwards.
suite('BraveAppearanceTabsFullscreen', () => {
  test('vertical tabs toggle defers the switch', async () => {
    await PrefService.getInstance().whenInitialized()
    document.body.replaceChildren()
    const tabs = document.createElement('settings-brave-appearance-tabs')
    document.body.appendChild(tabs)
    // Resolves after the page's own request, so the page also knows that the
    // tab strip can't be switched now.
    assertFalse(
        await sendWithPromise<boolean>('getIsVerticalTabsToggleEnabled'))
    await microtasksFinished()

    const toggle = tabs.shadowRoot!.querySelector<SettingsToggleButtonElement>(
        'settings-toggle-button.first')
    assertTrue(!!toggle)
    assertFalse(toggle.checked)

    const changed = eventToPromise('settings-boolean-control-change', toggle)
    toggle.click()
    await changed
    // The switch is applied later, so the toggle keeps its current value.
    assertFalse(toggle.checked)
  })
})
