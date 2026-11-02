/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://getting-started/brave_education_app.js'

import {
  BraveBrowserCommandHandlerRemote,
  browserProxyFactory as browserCommandProxyFactory,
  Command,
} from 'chrome://resources/brave/brave_browser_command.mojom-webui.js'
import { BraveEducationProxyImpl } from 'chrome://getting-started/brave_education_proxy.js'
import type { BraveEducationAppElement } from 'chrome://getting-started/brave_education_app.js'
import { PageCallbackRouter } from 'chrome://getting-started/brave_education.mojom-webui.js'
import { assertEquals, assertFalse, assertTrue } from 'chrome://webui-test/chai_assert.js'
import { TestMock } from 'chrome://webui-test/test_mock.js'
import { eventToPromise, microtasksFinished } from 'chrome://webui-test/test_util.js'

import { TestBraveEducationPageHandler } from './test_brave_education_page_handler.js'

const FIXTURES = 'chrome://webui-test/brave/brave_education'

suite('BraveEducationAppTest', () => {
  let handler: TestBraveEducationPageHandler
  let commandHandler: TestMock<BraveBrowserCommandHandlerRemote> &
    BraveBrowserCommandHandlerRemote

  setup(() => {
    document.body.innerHTML = window.trustedTypes!.emptyHTML
    handler = new TestBraveEducationPageHandler()
    BraveEducationProxyImpl.setInstance({
      callbackRouter: new PageCallbackRouter(),
      handler,
    })
    commandHandler = TestMock.fromClass(BraveBrowserCommandHandlerRemote)
    browserCommandProxyFactory.setInstance({ handler: commandHandler })
  })

  async function createApp(url: string): Promise<BraveEducationAppElement> {
    handler.setServerUrl(url)
    const app = document.createElement('brave-education-app')
    document.body.appendChild(app)
    await handler.whenCalled('getServerUrl')
    await microtasksFinished()
    return app
  }

  function getIframe(app: BraveEducationAppElement) {
    return app.shadowRoot.querySelector<HTMLIFrameElement>('#content')
  }

  test('ShowsTheServerUrl', async () => {
    const url = `${FIXTURES}/test.html`
    const iframe = getIframe(await createApp(url))
    assertTrue(!!iframe)
    assertEquals(url, iframe.src)
  })

  test('ShowsNothingWithoutAServerUrl', async () => {
    assertFalse(!!getIframe(await createApp('')))
  })

  test('RunsACommandTheBrowserCanExecute', async () => {
    commandHandler.setResultFor(
      'canExecuteCommand',
      Promise.resolve({ canExecute: true }),
    )
    await createApp(
      `${FIXTURES}/test_with_command.html?command=open-rewards-onboarding`,
    )
    assertEquals(
      Command.kOpenRewardsOnboarding,
      await commandHandler.whenCalled('canExecuteCommand'),
    )
    assertEquals(
      Command.kOpenRewardsOnboarding,
      await commandHandler.whenCalled('executeCommand'),
    )
  })

  test('SkipsACommandTheBrowserCannotExecute', async () => {
    commandHandler.setResultFor(
      'canExecuteCommand',
      Promise.resolve({ canExecute: false }),
    )
    await createApp(
      `${FIXTURES}/test_with_command.html?command=open-vpn-onboarding`,
    )
    assertEquals(
      Command.kOpenVPNOnboarding,
      await commandHandler.whenCalled('canExecuteCommand'),
    )
    await microtasksFinished()
    assertEquals(0, commandHandler.getCallCount('executeCommand'))
  })

  test('IgnoresCommandsFromOtherOrigins', async () => {
    await createApp(`${FIXTURES}/test.html`)
    const whenMessage = eventToPromise('message', window)
    window.postMessage(
      { messageType: 'browser-command', command: 'open-rewards-onboarding' },
      '*',
    )
    await whenMessage
    await microtasksFinished()
    assertEquals(0, commandHandler.getCallCount('canExecuteCommand'))
  })
})
