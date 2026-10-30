// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './appearance_page_test-chromium.js'

import {assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

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
