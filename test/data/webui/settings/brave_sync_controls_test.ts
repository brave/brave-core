// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Verifies the AI Chat sync toggle that Brave injects into the upstream
// settings-sync-controls element (see
// brave/browser/resources/settings/br/sync_controls.ts). The toggle is only
// added to the template when the kBraveSyncAIChat feature is enabled, which the
// accompanying browser test (brave_settings_browsertest.cc) turns on.

import 'chrome://settings/lazy_load.js';

import {webUIListenerCallback} from 'chrome://resources/js/cr.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import type {SettingsSyncControlsElement} from 'chrome://settings/lazy_load.js';
import type {CrToggleElement} from 'chrome://settings/settings.js';
import {SignedInState, StatusAction, SyncBrowserProxyImpl} from 'chrome://settings/settings.js';
import {assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js';
import {waitBeforeNextRender} from 'chrome://webui-test/polymer_test_util.js';
import {isVisible} from 'chrome://webui-test/test_util.js';

import {getSyncAllPrefs} from 'chrome://webui-test/settings/sync_test_util.js';
import {TestSyncBrowserProxy} from 'chrome://webui-test/settings/test_sync_browser_proxy.js';

suite('BraveSyncControlsAiChat', function() {
  let syncControls: SettingsSyncControlsElement;

  setup(async function() {
    SyncBrowserProxyImpl.setInstance(new TestSyncBrowserProxy());

    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    syncControls = document.createElement('settings-sync-controls');
    syncControls.syncStatus = {
      signedInState: SignedInState.SYNCING,
      statusAction: StatusAction.NO_ACTION,
    };
    document.body.appendChild(syncControls);

    webUIListenerCallback('sync-prefs-changed', getSyncAllPrefs());
    flush();
    await waitBeforeNextRender(syncControls);
  });

  function getAiChatToggle(): CrToggleElement|null {
    return syncControls.shadowRoot!.querySelector<CrToggleElement>(
        'cr-toggle[aria-labelledby="aiChatCheckboxLabel"]');
  }

  test('ToggleInjectedAndVisibleWhenRegistered', function() {
    const toggle = getAiChatToggle();
    assertTrue(
        !!toggle,
        'AI Chat sync toggle should be injected into settings-sync-controls');
    assertTrue(
        isVisible(toggle),
        'AI Chat sync toggle should be visible when aiChatRegistered is true');
    assertTrue(
        toggle.checked, 'Toggle should reflect aiChatSynced from sync prefs');
  });

  test('ToggleHiddenWhenNotRegistered', async function() {
    const prefs = getSyncAllPrefs();
    prefs.aiChatRegistered = false;
    webUIListenerCallback('sync-prefs-changed', prefs);
    flush();
    await waitBeforeNextRender(syncControls);

    assertFalse(
        isVisible(getAiChatToggle()),
        'AI Chat sync toggle should be hidden when aiChatRegistered is false');
  });
});
