/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

type EventName = 'open-modal'

interface EventHub {
  dispatch: (name: EventName, data: unknown) => void
  addListener: (name: EventName, fn?: (data: unknown) => void) => () => void
}

function globalEventName(name: string) {
  return `app-event-${name}`
}

export function createEventHub(): EventHub {
  const elem = document.createElement('div')
  return {
    dispatch(name, data) {
      const eventName = globalEventName(name)
      elem.dispatchEvent(new CustomEvent(eventName, { detail: data }))
    },
    addListener(name, fn) {
      const listener = (event: CustomEvent) => { fn && fn(event.detail) }
      const eventName = globalEventName(name)
      elem.addEventListener(eventName, listener)
      return () => elem.removeEventListener(eventName, listener)
    }
  }
}

// Allows dispatching of custom events within the React component tree. These
// events can be observed by any component within the same `EventHubContext`
// subtree.
export const EventHubContext = React.createContext(createEventHub())
