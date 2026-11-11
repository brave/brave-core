// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './reset_page_test-chromium.js'

import 'chrome://settings/lazy_load.js'

import type {SettingsResetProfileDialogElement} from 'chrome://settings/lazy_load.js'
import {ResetBrowserProxyImpl} from 'chrome://settings/settings.js'
import {
  assertDeepEquals,
  assertFalse,
  assertTrue,
} from 'chrome://webui-test/chai_assert.js'
import {isVisible, microtasksFinished} from 'chrome://webui-test/test_util.js'

import {TestResetBrowserProxy} from './test_reset_browser_proxy.js'

// Upstream's proxy drops `sendSettings`, which is what we care about.
class RecordingResetBrowserProxy extends TestResetBrowserProxy {
  sendSettingsValues: boolean[] = []

  override performResetProfileSettings(
      sendSettings: boolean, requestOrigin: string) {
    this.sendSettingsValues.push(sendSettings)
    return super.performResetProfileSettings(sendSettings, requestOrigin)
  }
}

// Brave never sends reset settings to Google. A lit_mangler unchecks, disables
// and hides the checkbox, so check that it keeps working.
suite('BraveResetProfileDialog', () => {
  let dialog: SettingsResetProfileDialogElement
  let resetBrowserProxy: RecordingResetBrowserProxy

  setup(async () => {
    resetBrowserProxy = new RecordingResetBrowserProxy()
    ResetBrowserProxyImpl.setInstance(resetBrowserProxy)

    document.body.replaceChildren()
    dialog = document.createElement('settings-reset-profile-dialog')
    document.body.appendChild(dialog)
    await microtasksFinished()
  })

  test('the send settings checkbox is unchecked, disabled and not shown', () => {
    const checkbox = dialog.$.sendSettings
    assertFalse(checkbox.checked)
    assertTrue(checkbox.disabled)
    assertFalse(isVisible(checkbox))
  })

  test('the footer holding the checkbox is hidden', () => {
    const footer = dialog.$.sendSettings.parentElement
    assertTrue(!!footer)
    assertTrue(footer.hidden)
  })

  test('resetting does not send settings', async () => {
    dialog.$.reset.click()
    await resetBrowserProxy.whenCalled('performResetProfileSettings')
    assertDeepEquals([false], resetBrowserProxy.sendSettingsValues)
  })
})
