/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { NewTabPageAdEventType } from '../../state/background_state'
import { AutocompleteMatch } from '../../state/search_state'

// Messages that can be sent to a rich media background iframe.
export type RichMediaOutgoingMessage =
  SafeRectMessage |
  SearchMatchesMessage

interface SafeRectMessage {
  type: 'richMediaSafeRect'
  value: {
    x: number,
    y: number,
    width: number,
    height: number
  }
}

interface SearchMatchesMessage {
  type: 'richMediaSearchMatches'
  value: AutocompleteMatch[]
}

// Messages that can be received from a rich media background iframe.
export type RichMediaIncomingMessage =
  AdEventMessage |
  SearchAutocompleteMessage |
  OpenSearchMessage

interface AdEventMessage {
  type: 'richMediaEvent'
  value: NewTabPageAdEventType
}

interface SearchAutocompleteMessage {
  type: 'richMediaQueryBraveSearchAutocomplete',
  value: string
}

interface OpenSearchMessage {
  type: 'richMediaOpenBraveSearchWithQuery',
  value: string
}

// Reads a rich media message posted from the background iframe. Exported for
// testing.
export function readIncomingMessage(
  data: any
): RichMediaIncomingMessage | null {
  if (!data) {
    return null
  }
  const { type } = data
  switch (type) {
    case 'richMediaEvent': {
      const value = parseAdEventType(data.value)
      return value !== null ? { type, value } : null
    }
    case 'richMediaQueryBraveSearchAutocomplete': {
      return data.value ? { type, value: String(data.value) } : null
    }
    case 'richMediaOpenBraveSearchWithQuery': {
      return data.value ? { type, value: String(data.value) } : null
    }
  }
  return null
}

function parseAdEventType(data: any): NewTabPageAdEventType | null {
  switch (data) {
    case 'click': return NewTabPageAdEventType.kClicked
    case 'interaction': return NewTabPageAdEventType.kInteraction
    case 'mediaPlay': return NewTabPageAdEventType.kMediaPlay
    case 'media25': return NewTabPageAdEventType.kMedia25
    case 'media100': return NewTabPageAdEventType.kMedia100
  }
  return null
}

// Allows posting rich media messages to a rich media background frame.
export interface RichMediaFrameHandle {
  postMessage: (message: RichMediaOutgoingMessage) => void
}

// The set of capabilities exposed to the rich media background iframe.
export interface RichMediaCapabilities {
  notifyAdEvent: (eventType: NewTabPageAdEventType) => void
  openDestinationUrl: () => void
  openBraveSearch: (pathAndQuery: string) => void
  queryBraveSearchAutocomplete: (query: string) => void
}

// Takes an incoming rich media message and executes the appropriate capability.
// Exported for testing. Clients should use `handleRawIncomingMessage`.
export function handleIncomingMessage(
  message: RichMediaIncomingMessage,
  capabilities: RichMediaCapabilities
) {
  switch (message.type) {
    case 'richMediaEvent': {
      capabilities.notifyAdEvent(message.value)
      if (message.value === NewTabPageAdEventType.kClicked) {
        capabilities.openDestinationUrl()
      }
      break
    }
    case 'richMediaOpenBraveSearchWithQuery': {
      capabilities.notifyAdEvent(NewTabPageAdEventType.kClicked)
      capabilities.openBraveSearch(message.value)
      break
    }
    case 'richMediaQueryBraveSearchAutocomplete': {
      capabilities.queryBraveSearchAutocomplete(message.value)
      break
    }
  }
}

// Takes a raw incoming rich media message and executes the appropriate
// capability.
export function handleRawIncomingMessage(
  data: unknown,
  capabilities: RichMediaCapabilities
) {
  const message = readIncomingMessage(data)
  if (!message) {
    console.warn('Invalid rich media message', data)
    return
  }
  handleIncomingMessage(message, capabilities)
}
