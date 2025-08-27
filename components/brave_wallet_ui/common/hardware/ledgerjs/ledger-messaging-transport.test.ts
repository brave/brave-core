/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LedgerMessagingTransport } from './ledger-messaging-transport'
import {
  LedgerCommand,
  LedgerBridgeErrorCodes,
  UnlockCommand,
} from './ledger-messages'
import { TrustedOrigins } from '../untrusted_shared_types'

// We must read and write to protected class attributes in the tests.
// That yields a typescript error unless we use bracket notation, e.g.
// `transport['handlers']` instead of `transport.handlers`. As a result we
// silence the dot-notation tslint rule for the file.
/* eslint-disable @typescript-eslint/dot-notation */

const createTransport = (
  targetUrl: string = 'chrome-untrusted://ledger-bridge',
): LedgerMessagingTransport => {
  const iframe = document.createElement('iframe')
  document.body.appendChild(iframe)
  if (!iframe.contentWindow) {
    fail('transport should be defined')
  }
  // Use Object.defineProperty in order to assign to
  // window.crypto because standard assignment results in
  // assignment error because window.origin is read-only
  Object.defineProperty(iframe.contentWindow, 'origin', {
    value: targetUrl,
  })
  const targetWindow = iframe.contentWindow
  const transport = new LedgerMessagingTransport(
    targetWindow,
    targetWindow.origin,
  )
  ;(transport as any)['senderWindow'] = window
  return transport
}

test('constructor', () => {
  const transport = new LedgerMessagingTransport(window, window.origin)

  expect(transport['targetWindow']).toEqual(window)
  expect(transport['targetUrl']).toEqual(window.origin)
  expect(transport['handlers'].size).toEqual(0)
})

test('sendCommand configures handler for the response message ', async () => {
  const transport = createTransport()
  const sendEvent: UnlockCommand = {
    id: LedgerCommand.Unlock,
    origin: (transport as any)['senderWindow']['origin'],
    command: LedgerCommand.Unlock,
  }
  const initialHandlersCount = transport['handlers'].size
  transport['targetWindow'].postMessage = (eventData) => {
    // Sender should have created a handler to handle the reply
    expect(transport['handlers'].size).toEqual(initialHandlersCount + 1)
  }
  transport.sendCommand(sendEvent)
})

test('sendCommand returns CommandInProgress if response handler already exists', async () => {
  const transport = createTransport()
  const sendEvent: UnlockCommand = {
    id: LedgerCommand.Unlock,
    origin: (transport as any)['senderWindow'].origin,
    command: LedgerCommand.Unlock,
  }

  transport['targetWindow'].postMessage = (eventData) => {
    // Sending a second message before the first is replied to should result in
    // CommandInProgress
    transport.sendCommand(sendEvent).then((inflightResponse) => {
      expect(inflightResponse).toEqual(LedgerBridgeErrorCodes.CommandInProgress)
    })
  }
  transport.sendCommand(sendEvent) // Send the message
})

test('sendCommand removes command handler for the response when its resolved', async () => {
  const transport = createTransport()
  const sendEvent: UnlockCommand = {
    id: LedgerCommand.Unlock,
    origin: (transport as any)['senderWindow']['origin'],
    command: LedgerCommand.Unlock,
  }

  transport['targetWindow'].postMessage = (eventData) => {
    const replyEvent: MessageEvent = new MessageEvent('message', {
      data: eventData,
      origin: transport['targetWindow'].origin,
      source: transport['targetWindow'],
    })
    ;(transport as any)['senderWindow'].dispatchEvent(replyEvent)
  }

  await transport.sendCommand(sendEvent)

  // Reply handler should be removed when sendCommand resolves
  expect(transport['handlers'].size).toEqual(0)
})

test('onMessageReceived ignores messages not from the targetUrl', () => {
  const transport = createTransport()
  const testId = 'test'
  let eventData = {
    id: testId,
    command: testId,
  }

  let callbackCalled = false
  // Events nott from the targetUrl should not be handled
  const invalidEvent: MessageEvent = new MessageEvent('message', {
    data: eventData,
    origin: (transport as any)['senderWindow']['origin'],
    source: (transport as any)['senderWindow'],
  })
  transport['addCommandHandler'](testId, () => (callbackCalled = true))
  ;(transport as any)['senderWindow'].dispatchEvent(invalidEvent)
  expect(callbackCalled).toEqual(false)

  // Events from the targetUrl should be handled
  const validEvent: MessageEvent = new MessageEvent('message', {
    data: eventData,
    origin: transport['targetWindow'].origin,
    source: transport['targetWindow'],
  })
  ;(transport as any)['senderWindow'].dispatchEvent(validEvent)
  expect(callbackCalled).toEqual(true)
})

test('onMessageReceived invokes handler and replies with response', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    origin: transport['targetWindow'].origin,
    command: testId,
  }
  const event: MessageEvent = new MessageEvent('message', {
    data: eventData,
    // event.origin === event.data.origin when a response is expected
    origin: eventData.origin,
    source: transport['targetWindow'],
  })

  const expectedResponse = 123
  transport['addCommandHandler'](testId, () => {
    return expectedResponse
  })

  transport['targetWindow'].postMessage = (event) => {
    expect(event).toEqual(expectedResponse)
  }
  ;(transport as any)['senderWindow'].dispatchEvent(event)
})

test('onMessageReceived does not reply with response if the receiving message is already response', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    origin: transport['targetWindow'].origin,
    command: testId,
  }
  let callbackCalled = false
  const event: MessageEvent = new MessageEvent('message', {
    data: eventData,
    // event.origin !== event.data.origin when it
    // is a reponse to a sendCommand
    origin: (transport as any)['senderWindow']['origin'],
    source: transport['targetWindow'],
  })
  transport['addCommandHandler'](testId, () => {
    callbackCalled = true
  })
  ;(transport as any)['senderWindow'].dispatchEvent(event)
  expect(callbackCalled).toEqual(false)
})

test('should accept messages from trusted origins', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    command: testId,
  }

  let callbackCalled = false
  transport['addCommandHandler'](testId, () => (callbackCalled = true))

  // Test all trusted origins
  TrustedOrigins.forEach((trustedOrigin) => {
    const validEvent: MessageEvent = new MessageEvent('message', {
      data: eventData,
      origin: trustedOrigin,
      source: transport['targetWindow'],
    })

    // Reset callback state
    callbackCalled = false
    ;(transport as any)['senderWindow'].dispatchEvent(validEvent)

    // Should accept messages from trusted origins
    expect(callbackCalled).toEqual(true)
  })
})

test('should reject messages from untrusted origins', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    command: testId,
  }

  let callbackCalled = false
  transport['addCommandHandler'](testId, () => (callbackCalled = true))

  // Test various untrusted origins
  const untrustedOrigins = [
    'https://evil.com',
    'http://malicious-site.com',
    'chrome://other-page',
    'chrome-untrusted://other-bridge',
    'file:///tmp/malicious.html',
    'data:text/html,<script>alert("xss")</script>',
    'javascript:alert("xss")',
    'chrome-extension://malicious-extension',
    'moz-extension://malicious-extension',
    'about:blank',
    'about:srcdoc',
  ]

  untrustedOrigins.forEach((untrustedOrigin) => {
    const invalidEvent: MessageEvent = new MessageEvent('message', {
      data: eventData,
      origin: untrustedOrigin,
      source: transport['targetWindow'],
    })

    // Reset callback state
    callbackCalled = false
    ;(transport as any)['senderWindow'].dispatchEvent(invalidEvent)

    // Should reject messages from untrusted origins
    expect(callbackCalled).toEqual(false)
  })
})

test('should reject messages with null or undefined origin', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    command: testId,
  }

  let callbackCalled = false
  transport['addCommandHandler'](testId, () => (callbackCalled = true))

  // Test null and undefined origins
  const invalidOrigins = [null, undefined, '']

  invalidOrigins.forEach((invalidOrigin) => {
    const invalidEvent: MessageEvent = new MessageEvent('message', {
      data: eventData,
      origin: invalidOrigin as any,
      source: transport['targetWindow'],
    })

    // Reset callback state
    callbackCalled = false
    ;(transport as any)['senderWindow'].dispatchEvent(invalidEvent)

    // Should reject messages with invalid origins
    expect(callbackCalled).toEqual(false)
  })
})

test('should reject messages without source', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    command: testId,
  }

  let callbackCalled = false
  transport['addCommandHandler'](testId, () => (callbackCalled = true))

  // Test message without source
  const invalidEvent: MessageEvent = new MessageEvent('message', {
    data: eventData,
    origin: TrustedOrigins[0], // Use trusted origin
    source: null, // No source
  })

  ;(transport as any)['senderWindow'].dispatchEvent(invalidEvent)

  // Should reject messages without source
  expect(callbackCalled).toEqual(false)
})

test('should reject non-message events', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    command: testId,
  }

  let callbackCalled = false
  transport['addCommandHandler'](testId, () => (callbackCalled = true))

  // Test non-message events
  const nonMessageEvents = ['load', 'error', 'focus', 'blur']

  nonMessageEvents.forEach((eventType) => {
    const invalidEvent: MessageEvent = new MessageEvent(eventType as any, {
      data: eventData,
      origin: TrustedOrigins[0], // Use trusted origin
      source: transport['targetWindow'],
    })

    // Reset callback state
    callbackCalled = false
    ;(transport as any)['senderWindow'].dispatchEvent(invalidEvent)

    // Should reject non-message events
    expect(callbackCalled).toEqual(false)
  })
})

test('should handle edge case with malformed event data', () => {
  const transport = createTransport()
  const testId = 'test'

  let callbackCalled = false
  transport['addCommandHandler'](testId, () => (callbackCalled = true))

  // Test with malformed event data
  const malformedEvents = [
    null,
    undefined,
    {},
    { id: 'test' }, // Missing command
    { command: 'test' }, // Missing id
  ]

  malformedEvents.forEach((malformedData) => {
    const invalidEvent: MessageEvent = new MessageEvent('message', {
      data: malformedData,
      origin: TrustedOrigins[0], // Use trusted origin
      source: transport['targetWindow'],
    })

    // Reset callback state
    callbackCalled = false
    ;(transport as any)['senderWindow'].dispatchEvent(invalidEvent)

    // Should handle malformed data gracefully
    expect(callbackCalled).toEqual(false)
  })
})

test('should handle case sensitivity in origin validation', () => {
  const transport = createTransport()
  const testId = 'test'
  const eventData = {
    id: testId,
    command: testId,
  }

  let callbackCalled = false
  transport['addCommandHandler'](testId, () => (callbackCalled = true))

  // Test case variations of trusted origins
  const caseVariations = [
    'CHROME-UNTRUSTED://LEDGER-BRIDGE',
    'Chrome-Untrusted://Ledger-Bridge',
    'chrome://WALLET',
    'Chrome://Wallet',
  ]

  caseVariations.forEach((caseVariation) => {
    const invalidEvent: MessageEvent = new MessageEvent('message', {
      data: eventData,
      origin: caseVariation,
      source: transport['targetWindow'],
    })

    // Reset callback state
    callbackCalled = false
    ;(transport as any)['senderWindow'].dispatchEvent(invalidEvent)

    // Should reject case variations (origin validation is case-sensitive)
    expect(callbackCalled).toEqual(false)
  })
})
