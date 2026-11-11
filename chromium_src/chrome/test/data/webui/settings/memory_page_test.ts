// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './memory_page_test-chromium.js'

import type {SettingsMemoryPageElement} from 'chrome://settings/settings.js'
import {
  MEMORY_SAVER_MODE_AGGRESSIVENESS_PREF,
  MEMORY_SAVER_MODE_PREF,
  MemorySaverModeAggressiveness,
  MemorySaverModeState,
  PerformanceMetricsProxyImpl,
  PrefService,
  PrefsBrowserProxy,
  TAB_DISCARD_EXCEPTIONS_MANAGED_PREF,
  TAB_DISCARD_EXCEPTIONS_PREF,
} from 'chrome://settings/settings.js'
import {assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestPerformanceMetricsProxy} from './test_performance_metrics_proxy.js'
import {TestPrefsBrowserProxy} from './test_prefs_browser_proxy.js'

const INITIAL_PREFS: chrome.settingsPrivate.PrefObject[] = [
  {
    key: MEMORY_SAVER_MODE_PREF,
    type: chrome.settingsPrivate.PrefType.NUMBER,
    value: MemorySaverModeState.DISABLED,
  },
  {
    key: MEMORY_SAVER_MODE_AGGRESSIVENESS_PREF,
    type: chrome.settingsPrivate.PrefType.NUMBER,
    value: MemorySaverModeAggressiveness.MEDIUM,
  },
  {
    key: TAB_DISCARD_EXCEPTIONS_PREF,
    type: chrome.settingsPrivate.PrefType.DICTIONARY,
    value: {},
  },
  {
    key: TAB_DISCARD_EXCEPTIONS_MANAGED_PREF,
    type: chrome.settingsPrivate.PrefType.LIST,
    value: [],
  },
]

// Brave puts the tab discard exception list, which upstream moved to a page we
// don't show, back at the top of the memory page. A lit_mangler inserts it and
// an override registers it, so check both, since they apply silently.
suite('BraveMemoryPage', () => {
  let memoryPage: SettingsMemoryPageElement

  setup(async () => {
    document.body.replaceChildren()

    PerformanceMetricsProxyImpl.setInstance(new TestPerformanceMetricsProxy())

    PrefsBrowserProxy.setInstance(new TestPrefsBrowserProxy(INITIAL_PREFS))
    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()

    memoryPage = document.createElement('settings-memory-page')
    document.body.appendChild(memoryPage)
    await microtasksFinished()
  })

  test('the exception list element is defined', () => {
    // Upstream's memory page doesn't import it.
    assertTrue(!!customElements.get('tab-discard-exception-list'))
  })

  test('the exception list is the first item of the page', () => {
    const section = memoryPage.shadowRoot.querySelector('settings-section')
    assertTrue(!!section)
    const list = section.firstElementChild
    assertTrue(!!list)
    assertEquals('tab-discard-exception-list', list.tagName.toLowerCase())
    assertEquals('exceptionList', list.id)
  })

  test('a separator follows the exception list', () => {
    const list = memoryPage.shadowRoot.querySelector('#exceptionList')
    assertTrue(!!list)
    const separator = list.nextElementSibling
    assertTrue(!!separator)
    assertTrue(separator.classList.contains('hr'))
  })

  test('the exception list renders', () => {
    const list = memoryPage.shadowRoot.querySelector('#exceptionList')
    assertTrue(!!list)
    assertTrue(!!list.shadowRoot)
    assertTrue(list.shadowRoot.childElementCount > 0)
  })
})
