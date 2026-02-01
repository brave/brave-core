/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { TrezorBridgeTransport } from './trezor-bridge-transport'
import {
  kTrezorBridgeUrl,
  TrezorCommand,
  TrezorErrorsCodes,
  TrezorFrameCommand,
} from './trezor-messages'

// Mock loadTimeData.getString to return proper chrome URL
jest.mock('../../../../common/loadTimeData', () => ({
  loadTimeData: {
    getString: jest.fn((key: string) => {
      if (key === 'braveWalletTrezorBridgeUrl') {
        return 'chrome-untrusted://trezor-bridge'
      }
      return key
    }),
  },
}))

// Mock crypto.randomUUID
Object.defineProperty(global.crypto, 'randomUUID', {
  value: () => 'test-frame-id-123',
  writable: true,
})

// We must read and write to protected class attributes in the tests.
// That yields a typescript error unless we use bracket notation, e.g.
// `transport['handlers']` instead of `transport.handlers`. As a result we
// silence the dot-notation tslint rule for the file.

const createTransport = (
  targetUrl: string = 'chrome-untrusted://trezor-bridge',
): TrezorBridgeTransport => {
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  if (!iframe.contentWindow) {
    fail('transport should be defined')
  }
  // Use Object.defineProperty in order to assign to
  // iframe.contentWindow.origin because standard assignment results in
  // assignment error because origin is read-only
  Object.defineProperty(iframe.contentWindow, 'origin', {
    value: targetUrl,
  })
  const transport = new TrezorBridgeTransport(targetUrl)
  transport['bridge'] = iframe
  return transport
}

test('constructor', () => {
  const transport = new TrezorBridgeTransport(kTrezorBridgeUrl)

  expect(transport['frameId']).toBeDefined()
  expect(transport['bridgeFrameUrl']).toEqual(kTrezorBridgeUrl)
  expect(transport['handlers'].size).toEqual(0)
})

test(`sendCommandToTrezorFrame configures handler for the
   response message`, async () => {
  const transport = createTransport()
  const sendEvent: TrezorFrameCommand = {
    id: 'test-unlock',
    origin: 'chrome://wallet',
    command: TrezorCommand.Unlock,
  }
  const initialHandlersCount = transport['handlers'].size
  transport['bridge']!.contentWindow!.postMessage = (eventData) => {
    // Sender should have created a handler to handle the reply
    expect(transport['handlers'].size).toEqual(initialHandlersCount + 1)
  }
  transport.sendCommandToTrezorFrame(sendEvent)
})

test(`sendCommandToTrezorFrame returns CommandInProgress if response handler
   already exists`, async () => {
  const transport = createTransport()
  const sendEvent: TrezorFrameCommand = {
    id: 'test-unlock',
    origin: 'chrome://wallet',
    command: TrezorCommand.Unlock,
  }

  transport['bridge']!.contentWindow!.postMessage = (eventData) => {
    // Sending a second message before the first is replied to should result in
    // CommandInProgress
    transport.sendCommandToTrezorFrame(sendEvent).then((inflightResponse) => {
      expect(inflightResponse).toEqual(TrezorErrorsCodes.CommandInProgress)
    })
  }
  transport.sendCommandToTrezorFrame(sendEvent) // Send the message
})

test(`sendCommandToTrezorFrame removes command handler for the response
   when its resolved`, async () => {
  const transport = createTransport()
  const sendEvent: TrezorFrameCommand = {
    id: 'test-unlock',
    origin: 'chrome://wallet',
    command: TrezorCommand.Unlock,
  }

  transport['bridge']!.contentWindow!.postMessage = (eventData) => {
    const replyEvent: MessageEvent = new MessageEvent('message', {
      data: eventData,
      origin: transport['getTrezorBridgeOrigin'](),
      source: transport['bridge']!.contentWindow,
    })
    // Simulate async response
    setTimeout(() => transport['onMessageReceived'](replyEvent), 0)
  }

  await transport.sendCommandToTrezorFrame(sendEvent)

  // Reply handler should be removed when sendCommandToTrezorFrame resolves
  expect(transport['handlers'].size).toEqual(0)
})

test('onMessageReceived ignores messages not from the targetUrl', () => {
  const transport = createTransport()
  const testId = 'test'
  let eventData = {
    id: testId,
    command: TrezorCommand.Unlock,
  }

  let callbackCalled = false
  // Events not from the trezor bridge origin should not be handled
  const invalidEvent: MessageEvent = new MessageEvent('message', {
    data: eventData,
    origin: 'https://malicious-site.com',
    source: transport['bridge']!.contentWindow,
  })
  transport['addCommandHandler'](testId, () => (callbackCalled = true))
  transport['onMessageReceived'](invalidEvent)
  expect(callbackCalled).toEqual(false)

  // Events from the trezor bridge origin should be handled
  const validEvent: MessageEvent = new MessageEvent('message', {
    data: eventData,
    origin: transport['getTrezorBridgeOrigin'](),
    source: transport['bridge']!.contentWindow,
  })
  transport['onMessageReceived'](validEvent)
  expect(callbackCalled).toEqual(true)
})

test('onMessageReceived invokes handler and processes response', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    origin: transport['getTrezorBridgeOrigin'](),
    command: TrezorCommand.Unlock,
  }
  const event: MessageEvent = new MessageEvent('message', {
    data: eventData,
    // event.origin === event.data.origin when a response is expected
    origin: eventData.origin,
    source: transport['bridge']!.contentWindow,
  })

  let handlerCalled = false
  transport['addCommandHandler'](testId, (message: TrezorFrameCommand) => {
    expect(message).toEqual(eventData)
    handlerCalled = true
  })

  transport['onMessageReceived'](event)
  expect(handlerCalled).toEqual(true)
})

test(`onMessageReceived does not process response if the receiving message is
  from different origin`, () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    origin: 'chrome://test-origin',
    command: TrezorCommand.Unlock,
  }
  let callbackCalled = false
  const event: MessageEvent = new MessageEvent('message', {
    data: eventData,
    // event.origin !== trezor bridge origin when it
    // is from an untrusted source
    origin: 'chrome://different-origin',
    source: transport['bridge']!.contentWindow,
  })
  transport['addCommandHandler'](testId, () => {
    callbackCalled = true
  })
  transport['onMessageReceived'](event)
  expect(callbackCalled).toEqual(false)
})

test('onMessageReceived ignores messages when no handlers exist', () => {
  const transport = createTransport()
  const eventData = {
    id: 'test',
    command: TrezorCommand.Unlock,
  }
  const event: MessageEvent = new MessageEvent('message', {
    data: eventData,
    origin: transport['getTrezorBridgeOrigin'](),
    source: transport['bridge']!.contentWindow,
  })

  // Should return early when no handlers exist
  expect(() => transport['onMessageReceived'](event)).not.toThrow()
})
