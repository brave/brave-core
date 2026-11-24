/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { assert } from 'chai'
import { it } from 'mocha'

import * as SDK from '../../core/sdk/sdk.js'
import * as Protocol from '../../generated/protocol.js'
import { renderElementIntoDOM } from '../../testing/DOMHelpers.js'
import { describeWithEnvironment } from '../../testing/EnvironmentHelpers.js'
import { createTarget } from '../../testing/TargetHelpers.js'
import * as UI from '../../ui/legacy/legacy.js'

import { RequestAdblockView } from './RequestAdblockView.js'

interface AdblockDispatcher {
  requestAdblockInfoReceived(params: {
    requestId: string
    info?: object
  }): void
}

const REQUEST_ID = 'adblock-request' as Protocol.Network.RequestId
const REQUEST_URL = 'https://ads.example/ad.js'

const BLOCKED_INFO = {
  requestUrl: REQUEST_URL,
  checkedUrl: REQUEST_URL,
  sourceHost: 'site.example',
  aggressive: false,
  blocked: true,
  didMatchImportantRule: false,
  didMatchRule: true,
  didMatchException: false,
  hasMockData: false,
  filter: { ruleText: '||ads.example^' }
}

async function startRequest(): Promise<{
  request: SDK.NetworkRequest.NetworkRequest
  dispatcher: SDK.NetworkManager.NetworkDispatcher & AdblockDispatcher
}> {
  const manager = createTarget().model(SDK.NetworkManager.NetworkManager)!
  const started = manager.once(SDK.NetworkManager.Events.RequestStarted)
  manager.dispatcher.requestWillBeSent({
    requestId: REQUEST_ID,
    loaderId: 'loader' as Protocol.Network.LoaderId,
    documentURL: 'https://site.example/',
    request: { url: REQUEST_URL, method: 'GET', headers: {} },
    timestamp: 1,
    wallTime: 1,
    initiator: { type: Protocol.Network.InitiatorType.Other },
    type: Protocol.Network.ResourceType.Script
  } as Protocol.Network.RequestWillBeSentEvent)
  const { request } = await started
  return {
    request,
    dispatcher: manager.dispatcher as SDK.NetworkManager.NetworkDispatcher &
      AdblockDispatcher
  }
}

async function renderView(
  request: SDK.NetworkRequest.NetworkRequest
): Promise<RequestAdblockView> {
  const view = renderElementIntoDOM(new RequestAdblockView())
  view.request = request
  await UI.Widget.Widget.allUpdatesComplete
  return view
}

function rows(view: RequestAdblockView): string[][] {
  return [...view.contentElement.children].map((row) =>
    [...row.children].map((cell) => cell.textContent ?? '')
  )
}

describeWithEnvironment('RequestAdblockView', () => {
  it("renders a row for each field of the request's Adblock info", async () => {
    const { request, dispatcher } = await startRequest()
    dispatcher.requestAdblockInfoReceived({
      requestId: REQUEST_ID,
      info: BLOCKED_INFO
    })

    const view = await renderView(request)

    assert.includeDeepMembers(rows(view), [
      ['requestUrl', REQUEST_URL],
      ['blocked', 'true'],
      ['didMatchException', 'false'],
      ['filter', JSON.stringify(BLOCKED_INFO.filter)]
    ])
  })

  it('renders nothing for a request without Adblock info', async () => {
    const { request } = await startRequest()

    const view = await renderView(request)

    assert.isEmpty(rows(view))
  })

  it('refreshes when its request finishes loading', async () => {
    const { request, dispatcher } = await startRequest()
    const view = await renderView(request)
    dispatcher.requestAdblockInfoReceived({
      requestId: REQUEST_ID,
      info: BLOCKED_INFO
    })

    dispatcher.loadingFinished({
      requestId: REQUEST_ID,
      timestamp: 2,
      encodedDataLength: 0
    } as Protocol.Network.LoadingFinishedEvent)
    await UI.Widget.Widget.allUpdatesComplete

    assert.includeDeepMembers(rows(view), [['blocked', 'true']])
  })
})
