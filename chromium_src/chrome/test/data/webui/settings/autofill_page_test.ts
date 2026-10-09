// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './autofill_page_test-chromium.js'

import 'chrome://settings/settings.js'

import {
  AutofillManagerImpl,
  PaymentsManagerImpl,
} from 'chrome://settings/lazy_load.js'
import type {
  SettingsAutofillPageElement,
  SettingsToggleButtonElement,
} from 'chrome://settings/settings.js'
import {
  loadTimeData,
  MetricsBrowserProxyImpl,
  PasswordManagerImpl,
  PrefService,
  PrefsBrowserProxy,
  resetRouterForTesting,
  Router,
} from 'chrome://settings/settings.js'
import {
  assertEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestAutofillManager, TestPaymentsManager} from './autofill_fake_data.js'
import {TestMetricsBrowserProxy} from './test_metrics_browser_proxy.js'
import {TestPasswordManagerProxy} from './test_password_manager_proxy.js'
import {TestPrefsBrowserProxy} from './test_prefs_browser_proxy.js'

// Brave restyles the "Your saved info" page and adds to it with a
// lit_mangler and prototype wraps. Check the result, since the overrides apply
// silently if upstream changes the template.
suite('BraveAutofillPage', () => {
  let autofillPage: SettingsAutofillPageElement
  let prefsBrowserProxy: TestPrefsBrowserProxy
  let metricsBrowserProxy: TestMetricsBrowserProxy

  suiteSetup(async () => {
    prefsBrowserProxy = new TestPrefsBrowserProxy([
      {
        key: 'signin.allowed_on_next_startup',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: true,
      },
      {
        key: 'autofill.profile_enabled',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: true,
      },
      {
        key: 'brave.autofill_private_windows',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: false,
      },
    ])
    PrefsBrowserProxy.setInstance(prefsBrowserProxy)
    PrefService.resetInstanceForTesting()
    await PrefService.getInstance().whenInitialized()
  })

  async function createPage(
      overrides: {[key: string]: boolean|string} = {}) {
    loadTimeData.overrideValues(overrides)
    resetRouterForTesting()
    document.body.innerHTML = window.trustedTypes!.emptyHTML
    autofillPage = document.createElement('settings-autofill-page')
    document.body.appendChild(autofillPage)
    await microtasksFinished()
  }

  function query<T extends HTMLElement = HTMLElement>(selector: string): T {
    const element = autofillPage.shadowRoot.querySelector<T>(selector)
    assertTrue(!!element, `couldn't find ${selector}`)
    return element
  }

  setup(async () => {
    AutofillManagerImpl.setInstance(new TestAutofillManager())
    PasswordManagerImpl.setInstance(new TestPasswordManagerProxy())
    PaymentsManagerImpl.setInstance(new TestPaymentsManager())
    metricsBrowserProxy = new TestMetricsBrowserProxy()
    MetricsBrowserProxyImpl.setInstance(metricsBrowserProxy)
    await createPage()
  })

  test('hides the page title, account card and Autofill AI rows', () => {
    for (const selector of [
      '#title',
      '#subtitle',
      'settings-account-card',
      '#identityManagerButton',
      '#travelManagerButton',
      'collapsible-autofill-settings-card',
    ]) {
      assertTrue(query(selector).hidden, `${selector} should be hidden`)
    }
  })

  test('hides the related services and Autofill AI sections', () => {
    // `display: flex` on <settings-section> wins over `hidden`.
    for (const selector of [
      'collapsible-autofill-settings-card',
      '#googleAccountButton',
    ]) {
      const section = query(selector).closest('settings-section')
      assertTrue(!!section)
      assertEquals('none', getComputedStyle(section).display)
    }
  })

  test('keeps the hidden cards upstream resolves as controls', () => {
    assertTrue(!!query('#passwordManagerButton'))
    assertEquals(
        query('#identityManagerButton'),
        autofillPage.getAssociatedControlFor('identityDocs'))
    assertEquals(
        query('#travelManagerButton'),
        autofillPage.getAssociatedControlFor('travel'))
  })

  test('the private windows toggle is the last row and writes its pref',
       async () => {
         const toggle = query<SettingsToggleButtonElement>(
             '#autofillPrivateWindowsToggle')
         assertEquals(
             toggle, query('.card-container').lastElementChild)
         assertEquals('brave.autofill_private_windows', toggle.prefKey)
         assertFalse(toggle.checked)

         toggle.click()
         await microtasksFinished()
         assertTrue(toggle.checked)
         const pref = await prefsBrowserProxy.getPref(
             'brave.autofill_private_windows')
         assertEquals(true, pref.value)
       })

  test('the email aliases card is absent when the feature is off', async () => {
    await createPage({isEmailAliasesEnabled: false})
    assertEquals(null, autofillPage.shadowRoot.querySelector(
                           '#emailAliasesCard'))
  })

  // <if expr="enable_email_aliases">
  // The label is only registered when the feature is enabled.
  const enabledOverrides = {
    isEmailAliasesEnabled: true,
    SETTINGS_EMAIL_ALIASES_LABEL: 'Email aliases',
  }

  test('the email aliases card precedes payment methods', async () => {
    await createPage(enabledOverrides)
    const card = query('#emailAliasesCard')
    assertEquals(
        'Email aliases', (card as unknown as {cardTitle: string}).cardTitle)
    assertEquals(query('#paymentManagerButton'), card.nextElementSibling)
  })

  test('clicking the email aliases card navigates and stays off the ' +
       'upstream handler', async () => {
    await createPage(enabledOverrides)
    query('#emailAliasesCard').shadowRoot!
        .querySelector<HTMLElement>('cr-link-row')!.click()
    assertEquals(
        Router.getInstance().getRoutes().EMAIL_ALIASES,
        Router.getInstance().getCurrentRoute())
    // The upstream handler only knows about the upstream categories.
    assertEquals(0, metricsBrowserProxy.getCallCount('recordYourSavedInfoCategoryClick'))
  })

  test('the email aliases card is the control for its view', async () => {
    await createPage(enabledOverrides)
    assertEquals(
        query('#emailAliasesCard'),
        autofillPage.getAssociatedControlFor('email-aliases'))
  })
  // </if>
})
