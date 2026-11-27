// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './system_page_test-chromium.js'

import type {SettingsSystemPageElement} from 'chrome://settings/lazy_load.js'
import {loadTimeData} from 'chrome://settings/settings.js'
import {assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {isVisible, microtasksFinished} from 'chrome://webui-test/test_util.js'

// The gestures link row is only useful once the gestures feature has
// shipped; until then it must stay hidden so users can't configure gestures
// that do nothing. See `settings-gestures-page` for the page it links to.
suite('SystemPage gestures link row', () => {
  async function createPage(): Promise<SettingsSystemPageElement> {
    document.body.replaceChildren()
    const page = document.createElement('settings-system-page')
    document.body.appendChild(page)
    await microtasksFinished()
    return page
  }

  function getGesturesButton(page: SettingsSystemPageElement) {
    return page.shadowRoot.querySelector<HTMLElement>('#gesturesButton')
  }

  test('is shown when the gestures feature is enabled', async () => {
    loadTimeData.overrideValues({isGesturesFeatureEnabled: true})
    const page = await createPage()

    const gesturesButton = getGesturesButton(page)
    assertTrue(!!gesturesButton)
    assertTrue(isVisible(gesturesButton))
  })

  test('is hidden when the gestures feature is disabled', async () => {
    loadTimeData.overrideValues({isGesturesFeatureEnabled: false})
    const page = await createPage()

    const gesturesButton = getGesturesButton(page)
    assertTrue(!!gesturesButton)
    assertFalse(isVisible(gesturesButton))
  })
})
