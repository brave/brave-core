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

// Prefs that Brave's settings pages read when they render.
const B = chrome.settingsPrivate.PrefType.BOOLEAN
const N = chrome.settingsPrivate.PrefType.NUMBER
const S = chrome.settingsPrivate.PrefType.STRING

const pageInitialPrefs: Array<[string, chrome.settingsPrivate.PrefType,
                           boolean|number|string]> = [
  ['brave.ad_block.developer_mode', B, false],
  ['brave.ai_chat.context_menu_enabled', B, true],
  ['brave.ai_chat.tab_organization_enabled', B, true],
  ['brave.ai_chat.tab_organization_send_page_content', B, false],
  ['brave.brave_ads.sponsored.enabled', B, true],
  ['brave.brave_vpn.wireguard_enabled', B, false],
  ['brave.de_amp.enabled', B, true],
  ['brave.debounce.enabled', B, true],
  ['brave.enable_media_router_on_restart', B, false],
  ['brave.gcm.channel_status', B, false],
  ['brave.history_embeddings_enabled', B, false],
  ['brave.history.retention_days', N, 0],
  ['brave.psst.settings.enable_psst', B, true],
  ['brave.reduce_language', B, true],
  ['brave.request_otr.request_otr_action_option', N, 0],
  ['brave.shields.stats_badge_visible', B, true],
  ['brave.wallet.default_base_cryptocurrency', S, 'BTC'],
  ['brave.wallet.default_base_currency', S, 'USD'],
  ['brave.wallet.default_cardano_wallet', N, 0],
  ['brave.wallet.default_solana_wallet', N, 0],
  ['brave.wallet.default_wallet2', N, 0],
  ['brave.wallet.nft_discovery_enabled', B, false],
  ['brave.wallet.transaction_simulation_opt_in_status', N, 0],
  ['signin.allowed_on_next_startup', B, true],
  ['tor.onion_only_in_tor_windows', B, true],
  ['webrtc.ip_handling_policy', S, 'default'],
  ['brave.local_ai.on_device_speech_model_enabled', B, false],
  ['brave.shields.fb_embed_default', B, true],
]
const pagePrefObjects =
    pageInitialPrefs.map(([key, type, value]) => ({key, type, value}))

// Brave pages used to bind a computed `pref` object, which `settings-toggle-
// button` asserts against once it became Lit. Rendering each page surfaces
// those errors.
suite('BraveSettingsToggleButtonPagesRender', () => {
  const pages = [
    'settings-brave-data-collection-subpage',
    'settings-brave-default-extensions-page',
    'brave-extensions-v2-subpage',
    'settings-brave-leo-assistant-page',
    'settings-brave-personalization-options',
    'settings-brave-vpn-page',
    'settings-brave-tor-subpage',
    'settings-brave-wallet-page',
    'settings-default-brave-shields-page',
  ]

  setup(async () => {
    PrefsBrowserProxy.setInstance(new TestPrefsBrowserProxy(pagePrefObjects))
    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()
    document.body.replaceChildren()
  })

  for (const tag of pages) {
    test(`${tag} renders its toggles`, async function() {
      // Not every page is built on every platform (e.g. VPN on Linux).
      if (!customElements.get(tag)) {
        this.skip()
      }
      const page = document.createElement(tag)
      document.body.appendChild(page)
      await microtasksFinished()
      const toggles =
          page.shadowRoot!.querySelectorAll('settings-toggle-button')
      for (const toggle of toggles) {
        await toggle.updateComplete
      }
    })
  }
})

// Brave's Polymer pages `include="md-select"`, whose rules Brave overrides in
// chromium_src/chrome/browser/resources/settings_shared/md_select.css.
suite('BraveMdSelectStyle', () => {
  setup(async () => {
    PrefsBrowserProxy.setInstance(new TestPrefsBrowserProxy(pagePrefObjects))
    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()
    document.body.replaceChildren()
  })

  test('Polymer page selects use Brave\'s md-select style', async () => {
    const page = document.createElement('settings-default-brave-shields-page')
    document.body.appendChild(page)
    await microtasksFinished()

    const select =
        page.shadowRoot!.querySelector<HTMLSelectElement>('#adControlType')
    assertTrue(!!select)
    const style = getComputedStyle(select)
    // Brave's rules.
    assertEquals('base-select', style.appearance)
    assertEquals('none', style.backgroundImage)
    // Upstream's rules still apply underneath.
    assertEquals('pointer', style.cursor)
  })
})
