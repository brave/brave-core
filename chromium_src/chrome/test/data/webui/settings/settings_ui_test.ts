// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './settings_ui_test-chromium.js'

import type {SettingsUiElement} from 'chrome://settings/settings.js'
import {assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

// Brave wraps `settings-ui`'s lifecycle callbacks and injects a style. Check
// they still apply, since they silently stop if upstream changes the element.
suite('BraveSettingsUI', () => {
  let ui: SettingsUiElement

  setup(async () => {
    document.body.replaceChildren()
    ui = document.createElement('settings-ui')
    document.body.appendChild(ui)
    await microtasksFinished()
  })

  test('the alert center is the first node of the shadow root', () => {
    const alertCenter = ui.shadowRoot.firstElementChild!
    assertEquals('LEO-ALERTCENTER', alertCenter.tagName)
    assertEquals('fixed', (alertCenter as HTMLElement).style.position)
  })

  test('the alert center survives a re-render', async () => {
    ui.requestUpdate()
    await ui.updateComplete
    assertEquals(1, ui.shadowRoot.querySelectorAll('leo-alertcenter').length)
    assertEquals(
        'LEO-ALERTCENTER', ui.shadowRoot.firstElementChild!.tagName)
  })

  test('Brave styles are applied', () => {
    // The right filler only takes up space in Chromium's layout.
    const right = ui.shadowRoot.querySelector<HTMLElement>('#right')
    assertTrue(!!right)
    assertEquals('none', getComputedStyle(right).display)
  })
})
