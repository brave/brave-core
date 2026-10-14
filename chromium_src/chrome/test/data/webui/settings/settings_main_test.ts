// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './settings_main_test-chromium.js'

import {loadTimeData} from 'chrome://settings/settings.js'
import {
  assertDeepEquals,
  assertEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

// The parts of the Brave Origin elements the tests reach into.
interface OriginHandler {
  refreshPurchaseState(): Promise<{isPurchased: boolean}>
  getNeedsRestart(): Promise<{needsRestart: boolean}>
  getPolicyValue(key: string): Promise<{value: boolean|null}>
  setPolicyValue(key: string, value: boolean): Promise<{success: boolean}>
}

interface OriginToggle extends HTMLElement {
  policyKey: string
  inverted: boolean
  checked: boolean
  braveOriginHandler_: OriginHandler
  loadPolicyValue_(): Promise<void>
}

interface OriginPage extends HTMLElement {
  braveOriginHandler_: OriginHandler
  updateComplete: Promise<boolean>
  onResetToDefaultsClick_(): Promise<void>
}

class TestOriginHandler implements OriginHandler {
  isPurchased = true
  needsRestart = false
  policyValues = new Map<string, boolean>()
  setCalls: Array<{key: string, value: boolean}> = []

  refreshPurchaseState() {
    return Promise.resolve({isPurchased: this.isPurchased})
  }

  getNeedsRestart() {
    return Promise.resolve({needsRestart: this.needsRestart})
  }

  getPolicyValue(key: string) {
    return Promise.resolve({value: this.policyValues.get(key) ?? null})
  }

  setPolicyValue(key: string, value: boolean) {
    this.setCalls.push({key, value})
    this.policyValues.set(key, value)
    return Promise.resolve({success: true})
  }
}

// The Brave Origin page is Brave's own Lit element, so nothing upstream
// exercises it.
suite('BraveOriginPage', () => {
  let page: OriginPage
  let handler: TestOriginHandler

  async function createPage() {
    document.body.replaceChildren()
    page = document.createElement('settings-brave-origin-page') as unknown as
        OriginPage
    // The element binds the real handler when it is constructed.
    page.braveOriginHandler_ = handler
    document.body.appendChild(page)
    await settled()
  }

  // Let the handler's promises resolve and the page render.
  async function settled() {
    await microtasksFinished()
    await page.updateComplete
    await microtasksFinished()
  }

  function getToggles(): OriginToggle[] {
    return Array.from(page.shadowRoot!.querySelectorAll<OriginToggle>(
        'origin-toggle-button'))
  }

  function getToggle(id: string): OriginToggle {
    const toggle = page.shadowRoot!.querySelector<OriginToggle>(`#${id}`)
    assertTrue(!!toggle, `couldn't find #${id}`)
    return toggle
  }

  setup(() => {
    handler = new TestOriginHandler()
    // Branded builds are always purchased, without asking the handler.
    loadTimeData.overrideValues({isBraveOriginBrandedBuild: true})
  })

  test('the toggles are bound to their policies', async () => {
    await createPage()

    const p3a = getToggle('toggleP3AButton')
    assertEquals('BraveP3AEnabled', p3a.policyKey)
    assertFalse(p3a.inverted)

    const stats = getToggle('toggleStatsReportingButton')
    assertEquals('BraveStatsPingEnabled', stats.policyKey)
    assertFalse(stats.inverted)

    // <if expr="enable_brave_rewards">
    // The policy disables Rewards, so the toggle is inverted.
    const rewards = getToggle('toggleRewardsButton')
    assertEquals('BraveRewardsDisabled', rewards.policyKey)
    assertTrue(rewards.inverted)
    // </if>

    // <if expr="enable_ai_chat">
    assertEquals('BraveAIChatEnabled', getToggle('toggleLeoAiButton').policyKey)
    // </if>
  })

  test('every toggle names a policy', async () => {
    await createPage()
    const toggles = getToggles()
    assertTrue(toggles.length > 0)
    for (const toggle of toggles) {
      assertTrue(!!toggle.policyKey, `#${toggle.id} has no policy`)
    }
  })

  test('the onboarding is shown instead when not purchased', async () => {
    loadTimeData.overrideValues({isBraveOriginBrandedBuild: false})
    handler.isPurchased = false
    await createPage()

    assertTrue(
        !!page.shadowRoot!.querySelector('settings-brave-origin-onboarding'))
    assertEquals(0, getToggles().length)
  })

  test('the toggles are shown once purchased', async () => {
    loadTimeData.overrideValues({isBraveOriginBrandedBuild: false})
    handler.isPurchased = true
    await createPage()

    assertEquals(
        null,
        page.shadowRoot!.querySelector('settings-brave-origin-onboarding'))
    assertTrue(getToggles().length > 0)
  })

  test('the restart notice is only shown when a restart is needed',
       async () => {
         await createPage()
         assertEquals(null, page.shadowRoot!.querySelector('#needsRestart'))

         handler.needsRestart = true
         await createPage()
         assertTrue(!!page.shadowRoot!.querySelector('#needsRestart'))
         assertTrue(!!page.shadowRoot!.querySelector('#restartButton'))
       })

  test('reset to defaults turns off the enabled toggles', async () => {
    await createPage()

    // Reload the toggles from the test handler, so their state is known.
    // Non-inverted policies are on when true, inverted ones when false.
    handler.policyValues.set('BraveP3AEnabled', true)
    handler.policyValues.set('BraveStatsPingEnabled', false)
    // <if expr="enable_brave_rewards">
    handler.policyValues.set('BraveRewardsDisabled', false)
    // </if>
    for (const toggle of getToggles()) {
      toggle.braveOriginHandler_ = handler
      await toggle.loadPolicyValue_()
    }
    await settled()
    assertTrue(getToggle('toggleP3AButton').checked)
    assertFalse(getToggle('toggleStatsReportingButton').checked)

    handler.setCalls = []
    await page.onResetToDefaultsClick_()

    // Enabled toggles are turned off, accounting for inversion.
    const expected = [{key: 'BraveP3AEnabled', value: false}]
    // <if expr="enable_brave_rewards">
    expected.push({key: 'BraveRewardsDisabled', value: true})
    // </if>
    for (const call of expected) {
      assertTrue(
          handler.setCalls.some(
              (c) => c.key === call.key && c.value === call.value),
          `expected ${JSON.stringify(call)}`)
    }

    // Toggles that were already off are left alone.
    assertFalse(
        handler.setCalls.some((c) => c.key === 'BraveStatsPingEnabled'))

    // Nothing is turned on.
    const inverted = new Set(
        getToggles().filter((t) => t.inverted).map((t) => t.policyKey))
    for (const call of handler.setCalls) {
      assertEquals(inverted.has(call.key), call.value, call.key)
    }
    assertDeepEquals(false, handler.policyValues.get('BraveP3AEnabled'))
  })
})
