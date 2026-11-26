// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './history_synced_tabs_test-chromium.js'

import type {HistorySyncedDeviceManagerElement} from 'chrome://history/history.js'
import {
  BrowserProxyImpl,
  foreignSessionBrowserProxyFactory,
  HistorySignInState,
  SyncState,
} from 'chrome://history/history.js'
import {assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js'
import {microtasksFinished} from 'chrome://webui-test/test_util.js'

import {FakeForeignSessionPageHandler} from './fake_foreign_session_page_handler.js'
import {TestHistoryBrowserProxy} from './test_browser_proxy.js'
import {createSession, createWindow} from './test_util.js'

// Brave's synced tabs come from Brave Sync, with no Google account, so the
// identity state reads as signed out. A plaster on synced_device_manager.ts
// keeps the device list when it changes; check that it still applies, as it
// would silently stop doing so if upstream moved the clearing.
suite('BraveSyncedDevices', () => {
  let element: HistorySyncedDeviceManagerElement
  let testProxy: TestHistoryBrowserProxy

  function getCardCount(): number {
    return element.shadowRoot.querySelectorAll('history-synced-device-card')
        .length
  }

  async function changeIdentityState(
      signIn: HistorySignInState, tabsSync: SyncState) {
    testProxy.pageRemote.onIdentityStateChanged({
      signIn,
      tabsSync,
      historySync: SyncState.kTurnedOff,
    })
    await testProxy.pageRemote.$.flushForTesting()
    await microtasksFinished()
  }

  setup(async () => {
    document.body.replaceChildren()
    testProxy = new TestHistoryBrowserProxy()
    BrowserProxyImpl.setInstance(testProxy)
    const {instance} = foreignSessionBrowserProxyFactory.createForTest(
        new FakeForeignSessionPageHandler())
    foreignSessionBrowserProxyFactory.setInstance(instance)
    testProxy.setInitialIdentityState({
      signIn: HistorySignInState.kSignedIn,
      tabsSync: SyncState.kTurnedOn,
      historySync: SyncState.kTurnedOff,
    })

    element = document.createElement('history-synced-device-manager')
    element.searchTerm = ''
    element.configureSignInForTest({signInAllowed: true, guestSession: false})
    document.body.appendChild(element)
    await microtasksFinished()

    element.sessionList = [createSession(
        'Brave Sync device', [createWindow(['https://brave.com'])])]
    await microtasksFinished()
    assertEquals(1, getCardCount())
  })

  test('keeps the devices when the identity reads as signed out', async () => {
    await changeIdentityState(
        HistorySignInState.kSignedOut, SyncState.kTurnedOff)
    assertTrue(element.sessionList !== null)
    assertEquals(1, getCardCount())
  })

  test('keeps the devices when tab sync reads as off', async () => {
    await changeIdentityState(
        HistorySignInState.kSignedIn, SyncState.kTurnedOff)
    assertTrue(element.sessionList !== null)
    assertEquals(1, getCardCount())
  })
})
