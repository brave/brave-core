// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './settings_menu_test-chromium.js'

import type {SettingsMenuElement} from 'chrome://settings/settings.js'
import {
  loadTimeData,
  resetPageVisibilityForTesting,
  resetRouterForTesting,
} from 'chrome://settings/settings.js'
import {
  assertDeepEquals,
  assertEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

// Brave rebuilds the menu with a lit_mangler. Check the result, since the
// override applies silently if upstream changes the template.
suite('BraveSettingsMenu', () => {
  let menu: SettingsMenuElement

  async function createMenu() {
    resetRouterForTesting()
    document.body.replaceChildren()
    menu = document.createElement('settings-menu')
    document.body.appendChild(menu)
    await microtasksFinished()
  }

  function getItem(id: string): HTMLElement|null {
    return menu.shadowRoot.querySelector<HTMLElement>(`a#${id}`)
  }

  function getItemIds(): string[] {
    return Array.from(menu.shadowRoot.querySelectorAll<HTMLElement>(
                          '[role=menuitem]'))
        .map((item) => item.id)
  }

  setup(createMenu)

  teardown(() => {
    resetPageVisibilityForTesting()
  })

  test('Brave items follow People in order', () => {
    const ids = getItemIds()
    const braveItems = [
      'people',
      'getStarted',
      'origin',
      'appearance',
      'content',
      'shields',
      'privacy',
      'braveSync',
      'search',
      'extensions',
    ]
    // Wallet and Leo are only there when allowed, so ignore them here.
    assertDeepEquals(
        braveItems,
        ids.filter((id) => braveItems.includes(id)))

    // The optional items sit between Privacy and Sync.
    for (const id of ['braveWallet', 'leoAssistant']) {
      if (ids.includes(id)) {
        assertTrue(ids.indexOf('privacy') < ids.indexOf(id))
        assertTrue(ids.indexOf(id) < ids.indexOf('braveSync'))
      }
    }
  })

  test('Autofill is listed with the advanced items', () => {
    const ids = getItemIds()
    assertEquals(ids.indexOf('languages') - 1, ids.indexOf('autofill'))
  })

  test('Performance and the extensions link are removed', () => {
    assertEquals(null, getItem('performance'))
    assertEquals(null, getItem('extensionsLink'))
  })

  test('Brave items point to their pages', () => {
    assertEquals('/getStarted', getItem('getStarted')!.getAttribute('href'))
    assertEquals('/origin', getItem('origin')!.getAttribute('href'))
    assertEquals('/braveContent', getItem('content')!.getAttribute('href'))
    assertEquals('/shields', getItem('shields')!.getAttribute('href'))
    assertEquals('/braveSync', getItem('braveSync')!.getAttribute('href'))
    assertEquals('/extensions', getItem('extensions')!.getAttribute('href'))
  })

  test('Brave items follow page visibility', async () => {
    resetPageVisibilityForTesting({
      getStarted: false,
      origin: false,
      content: false,
      shields: false,
      braveSync: false,
    })
    await createMenu()
    for (const id of ['getStarted', 'origin', 'content', 'shields',
                      'braveSync']) {
      assertTrue(getItem(id)!.hidden, `#${id} should be hidden`)
    }

    resetPageVisibilityForTesting({
      getStarted: true,
      origin: true,
      content: true,
      shields: true,
      braveSync: true,
    })
    await createMenu()
    for (const id of ['getStarted', 'origin', 'content', 'shields',
                      'braveSync']) {
      assertFalse(getItem(id)!.hidden, `#${id} should be shown`)
    }
  })

  test('Leo is only rendered when it is visible', async () => {
    resetPageVisibilityForTesting({leoAssistant: false})
    await createMenu()
    assertEquals(null, getItem('leoAssistant'))
  })

  test('About shows the product version', () => {
    const about = getItem('about-menu')!
    assertEquals('/help', about.getAttribute('href'))
    assertEquals(
        `v ${loadTimeData.getString('braveProductVersion')}`,
        about.querySelector('.brave-about-menu-version')!.textContent.trim())
  })
})
